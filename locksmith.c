/*
 * vim: ts=8 sw=8
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <ftw.h>

#define	DIM(a)		( sizeof( (a) ) / sizeof( (a)[0] ) )

typedef	struct	{
	mode_t	d_del;
	mode_t	d_add;
	mode_t	f_del;
	mode_t	f_add;
} Perms_t;

typedef	enum	{
	Role_lock,
	Role_unlock,
	Role_mustbelast
} Role_t;

static	Role_t		role= Role_lock;

static	Perms_t		perms[ Role_mustbelast ] =	{
	[Role_lock] =	{
		.d_del = ( S_IWUSR | S_IWGRP | S_IWOTH ),
		.d_add = ( S_IRUSR | S_IRGRP | S_IROTH |
			   S_IXUSR | S_IXGRP | S_IXGRP ),
		.f_del = ( S_IWUSR | S_IWGRP | S_IWOTH ),
		.f_add = ( S_IRUSR | S_IRGRP | S_IROTH )
	},
	[Role_unlock] =	{
		.d_del = 0,
		.d_add = ( S_IRUSR | S_IRGRP | S_IROTH |
			   S_IWUSR | S_IWGRP | 0       |
			   S_IXUSR | S_IXGRP | S_IXGRP ),
		.f_del = 0,
		.f_add = ( S_IRUSR | S_IRGRP | S_IROTH )
	}
};

static	char *		me = "locksmith";
static	char *		spelling;
static	int		(*role_callback)( char const *, struct stat const *, int, struct FTW * );
static	int		role_callback_flags = (FTW_MOUNT | FTW_PHYS);
static	unsigned	nonfatal;
static	unsigned	verbosity;

#define	BANTER_IF(l)	if( (l) <= verbosity )

#if	0
static	void
banter(
	unsigned	level,
	char const *	fmt,
	...
)
{
	if( level <= verbosity )	{
		va_list		ap;

		va_start( ap, fmt );
		vfprintf( stderr, fmt, ap );
		va_end( ap );
		fprintf( stderr, ".\n" );
	}
}
#endif	/* NOPE */

static	int
common_callback(
	char const * const		fn,
	struct stat const * const	st,
	int				flags,
	struct FTW * const		fs
)
{
	int		stopScan;

	stopScan = 0;
	do	{
		mode_t const	orig = st->st_mode & 
					(S_IRWXU | S_IRWXG | S_IRWXO);
		mode_t		new;
		int		e;

		new = orig;
		/* Turn off undesired privileges			*/
		if( S_ISDIR( st->st_mode ) )	{
			new &= ~perms[ role ].d_del;
			new |=  perms[ role ].d_add;
		} else	{
			new &= ~perms[ role ].f_del;
			new |=  perms[ role ].f_add;
		}
		/* Modify the privileges				*/
		e = chmod( fn, new ) ? errno : 0;
		BANTER_IF( 1 )	{
			int	i;

			for( i = 1; i < fs->level; ++i )	{
				printf( "  " );
			}
			printf( "%04o\t%04o\t%s", orig, new, fn );
			if( e )	{
				printf( "; errno=%d (%s)", e, strerror( e ) );
			}
			printf( "\n" );
		}
	} while( 0 );
	return( stopScan );
}

static	int
process(
	char const *	fn
)
{
	int		result;

	result = -1;
	do	{
		struct stat	st;
		int		status;

		/* Make sure we can see this file			*/
		if( stat( fn, &st ) == -1 )	{
			fprintf(
				stderr,
				"%s: cannot stat '%s'; errno=%d (%s).\n",
				me,
				fn,
				errno,
				strerror( errno )
			);
			break;
		}
		/* Must be a directory					*/
		if( ! S_ISDIR( st.st_mode ) )	{
			fprintf(
				stderr,
				"%s: '%s' is not a directory.\n",
				me,
				fn
			);
			break;
		}
		/* Walk the tree in the proper style for this role	*/
		status = nftw( 
			fn,
			role_callback,
			64,
			role_callback_flags
		);
		if( status )	{
			result = status;
			break;
		}
		result = 0;
	} while( 0 );
	return( result );
}

int
main(
	int		argc,
	char * *	argv
)
{
	char *		bp;
	int		c;

	/* Figure out our process name, and assume that role		*/
	me = argv[ 0 ];
	if( (bp = strrchr( me, '/' )) != NULL )	{
		me = bp + 1;
	}
	if( (bp = strchr( me, '.' )) != NULL )	{
		*bp = '\0';
	}
	spelling = me;
	/* Process the command line arguments				*/
	opterr = 0;
	while( (c = getopt( argc, argv, "r:v" )) != EOF )	{
		switch( c )	{
		default:
			fprintf(
				stderr, 
				"%s: -%c not implemented yet!\n", 
				me, 
				c 
			);
			exit( 1 );
		case '?':
			fprintf(
				stderr,
				"%s: unknown -%c switch!\n",
				me,
				optopt
			);
			exit( 1 );
		case 'r':
			spelling = optarg;
			break;
		case 'v':
			++verbosity;
			break;
		}
	}
	/* Selection the action routine based on our role		*/
	if( !strcmp( spelling, "lock" ) )	{
		role = Role_lock;
		role_callback = common_callback;
		role_callback_flags |= FTW_DEPTH;
	} else if( !strcmp( spelling, "unlock" ) )	{
		role = Role_unlock;
		role_callback = common_callback;
	} 
	if( ! role_callback )	{
		fprintf(
			stderr,
			"%s: unknown role '%s'.\n",
			me,
			spelling
		);
		exit( 1 );
	}
	/* Take targets from the command line, if we have any		*/
	if( optind < argc )	{
		while( optind < argc )	{
			optarg = argv[ optind++ ];

			if( process( optarg ) )	{
				++nonfatal;
			}
		}
	} else	{
		/* Read targets from stdin				*/
		int const	prompt = isatty( fileno( stdin ) );
		char		buf[ BUFSIZ ];

		setbuf( stdout, NULL );
		while( !feof( stdin ) )	{
			char *		eos;

			if( prompt )	{
				printf( "%s> ", spelling );
			}
			if( !fgets( buf, sizeof( buf ), stdin ) )	{
				if( prompt )	{
					printf( "\nEOF!\n" );
					break;
				}
			}
			/* Trim surrounding whitespace			*/
			for( bp = buf; *bp && isspace( *bp ); ++bp );
			for( 
				eos = bp + strlen( bp ); 
				(eos > bp) && isspace( eos[-1] );
				*--eos = '\0'
			);
			/* Deal with what we have			*/
			if( process( bp ) )	{
				++nonfatal;
			}
		}
	}
	return( nonfatal ? 1 : 0 );
}
