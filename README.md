# LOCKSMITH

## What

Locksmith is a small program which illustrates many progreamming techniques.

Externally, this program accepts one or more directory names and either locks or unlocks the directory and all its files.  The "locking" is simply adding or removing world access permissions.

For example, a "locked" directory looks like this:

	$ lock foo
	$ ls -ld foo
	drwxr-x---. 1 user user 112 Mar  3 22:09 foo

An "unlocked" looks like this:

	$ unlock foo
	$ ld -ld foo
	drwxr-xrwx. 1 user user 112 Mar  3 22:09 foo

## Why

The goal of the code is to illustrate details of how the gcc(1) compiler features mesh with kernel coding techniques.

N.B., the coding style is my own.  Please refer to the kernel documentation for the details of how the kernel code is structured.

## Points To Ponder

1. FIXME
1. FIXME MORE
