/***************************************************************************
                          tools.c  -  description
                             -------------------
    begin                : Fri Jan 19 2001
    copyright            : (C) 2001 by Michael Speck
    email                : kulkanie@gmx.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tools.h"

/* compares to strings and returns true if their first strlen(str1) chars are equal */
int strequal( const char *str1, const char *str2 )
{
    if ( strlen( str1 ) != strlen( str2 ) ) return 0;
    return ( !strncmp( str1, str2, strlen( str1 ) ) );
}

/* set delay to ms milliseconds */
void delay_set( Delay *delay, int ms )
{
    delay->limit = ms;
    delay->cur = 0;
}

/* reset delay ( cur = 0 )*/
void delay_reset( Delay *delay )
{
    delay->cur = 0;
}

/* check if times out and reset */
int delay_timed_out( Delay *delay, int ms )
{
    delay->cur += ms;
    if ( delay->cur >= delay->limit ) {

        delay->cur = 0;
        return 1;

    }
    return 0;
}


void goto_tile( int *x, int *y, int d )
{
    /*  0 -up, clockwise, 5 - left up */
    switch ( d ) {

        case 1:
            if ( !( (*x) & 1 ) )
                (*y)--;
            (*x)++;
            break;
        case 2:
            if ( (*x) & 1 )
                (*y)++;
            (*x)++;
            break;
        case 4:
            if ( (*x) & 1 )
                (*y)++;
            (*x)--;
            break;
        case 5:
            if ( !( (*x) & 1 ) )
                (*y)--;
            (*x)--;
            break;

    }
}

/* return distance between to map positions */
int get_dist( int x1, int y1, int x2, int y2 )
{
    int range = 0;

    while ( x1 != x2 || y1 != y2 ) {

        /* approach to x2,y2 */
        /*  0 -up, clockwise, 5 - left up */
        if ( y1 < y2 ) {

            if ( x1 < x2 )
                goto_tile( &x1, &y1, 2 );
            else
                if ( x1 > x2 )
                    goto_tile( &x1, &y1, 4 );
                else
                    y1++;

        }
        else
            if ( y1 > y2 ) {

                if ( x1 < x2 )
                    goto_tile( &x1, &y1, 1 );
                else
                    if ( x1 > x2 )
                        goto_tile( &x1, &y1, 5 );
                    else
                        y1--;

            }
            else {

                if ( x1 < x2 )
                    x1++;
                else
                    if ( x1 > x2 )
                        x1--;

            }

        range++;
    }

    return range;
}

/* init random seed by using ftime */
void set_random_seed()
{
    srand( (unsigned int)time( 0 ) );
}

/* get coordinates from string */
void get_coord( char *str, int *x, int *y )
{
    int i;
    char *cur_arg = 0;

    *x = *y = 0;

    /* get position of comma */
    for ( i = 0; i < strlen( str ); i++ )
        if ( str[i] == ',' ) break;
    if ( i == strlen( str ) ) {
        fprintf( stderr, "get_coord: no comma found in pair of coordinates '%s'\n", str );
        return; /* no comma found */
    }

    /* y */
    cur_arg = str + i + 1;
    if ( cur_arg[0] == 0 )
        fprintf( stderr, "get_coord: warning: y-coordinate is empty (maybe you left a space between x and comma?)\n" );
    *y = atoi( cur_arg );
    /* x */
    cur_arg = strdup( str ); cur_arg[i] = 0;
    *x = atoi( cur_arg );
    FREE( cur_arg );
}

/* replace new_lines with spaces in text */
void repl_new_lines( char *text )
{
    int i;
    for ( i = 0; i < strlen( text ); i++ )
        if ( text[i] < 32 )
            text[i] = 32;
}

// convert a str into text ( for listbox ) //
// char width is the width of a line in characters //
Text* create_text( char *orig_str, int char_width )
{
    int i, j;
    char line[256]; /* a line should not exceed this length */
    int pos;
    int last_space;
    int new_line;
    Text *text = 0;
    char *str = 0;

    text = calloc ( 1, sizeof( Text ) );

    // maybe orig_str is a constant expression; duplicate for safety //
    str = strdup( orig_str );

    // replace original new_lines with spaces //
    repl_new_lines( str );

    /* change some spaces to new_lines, so that the new text fits the wanted line_length */
    /* NOTE: '#' means new_line ! */
    // if character width is 0 it's just a single line //
    if ( char_width > 0 ) {
        pos = 0;
        while ( pos < strlen( str ) ) {
            last_space = 0;
            new_line = 0;
            i = 0;
            while ( !new_line && i < char_width && i + pos < strlen( str ) ) {
                switch ( str[pos + i] ) {
                    case '#': new_line = 1;
                    case 32: last_space = i; break;
                }
                i++;
            }
            if ( i + pos >= strlen( str ) ) break;
            if ( last_space == 0 ) {
                /* uhh... much to long, we'll have to cut a word into pieces ... */
                last_space = char_width / 2;
            }
            str[pos + last_space] = 10;
            pos += last_space;
        }
    }

    /* count lines */
    if ( char_width > 0 ) {
        for ( i = 0; i < strlen( str ); i++ )
            if ( str[i] == 10 )
                text->count++;
        /* maybe one unfinished line */
        if ( str[strlen( str ) - 1] != 10 )
            text->count++;
    }
    else
        text->count = 1;

    /* get mem */
    text->lines = calloc( text->count, sizeof( char* ) );

    pos = 0;
    /* get all lines */
    for ( j = 0; j < text->count; j++ ) {
        i = 0;
        while ( pos + i < strlen( str ) && str[pos + i] != 10 ) {
            line[i] = str[i + pos];
            i++;
        }
        pos += i; pos++;
        line[i] = 0;
        text->lines[j] = strdup( line );
    }

    if ( text->count == 0 )
        fprintf( stderr, "conv_to_text: warning: line_count is 0\n" );

    free( str );

    return text;
}

// delete text //
void delete_text( Text *text )
{
    int i;

    if ( text == 0 ) return;
/*
    if ( lines[1][1] == 'e' && lines[1][0] == '<' )
        printf( "hallo\n" );

    printf( "--- deleting:\n" );
    for ( i = 0; i < line_count; i++ ) {
        printf( lines[i] );
        printf( "\n" );
    }*/
    if ( text->lines ) {
        for ( i = 0; i < text->count; i++ )
            if ( text->lines[i] )
                free( text->lines[i] );
        free( text->lines );
    }
    free( text );
}

/*
====================================================================
Get type and prefix from string:
    type::prefix
Set both pointers 0 if failure.
====================================================================
*/
void get_type_and_prefix( char *arg, char **type, char **prefix )
{
    char *first, *second;

    *type = *prefix = 0;

    first = strtok( arg, ":" );
    second = strtok( 0, ":" );

    if ( first == 0 || second == 0 ) return;

    *type = strdup( first );
    *prefix = strdup( second );
}

/*
====================================================================
Replace any existence of character old into new.
====================================================================
*/
void strrepl( char **str, char c_old, char c_new )
{
    char *pos;
    while ( ( pos = strchr( *str, c_old ) ) != 0 )
        pos[0] = c_new;
}

/*
====================================================================
Copy source to dest and at maximum limit chars. Terminate with 0.
====================================================================
*/
void strcpy_lt( char *dest, char *src, int limit )
{
    int len = strlen( src );
    if ( len > limit ) {
        strncpy( dest, src, limit );
        dest[limit] = 0;
    }
    else
        strcpy( dest, src );
}

/*
====================================================================
Parse a version string and return the major version and the current
update.
====================================================================
*/
void parse_version( char *string, int *version, int *update )
{
    char *ptr = strchr( string, '.' );
    if ( ptr ) ptr[0] = 0;
    *version = atoi( string );
    if ( ptr ) {
        ptr++;
        *update = atoi( ptr );
        if ( *update < 10 && ptr[0] != '0' )
            *update *= 10; /* allow stuff like 1.01 */
        ptr--;
        ptr[0] = '.';
    }
    else
        *update = 0;
}

/* allocate memory or exit with error if out of it */
void *salloc( int num, int size )
{
	void *ptr = calloc( num, size );
	if ( ptr == 0 ) {
		printf( "out of memory\n" );
		exit(1);
	}
	return ptr;
}

/* print contents of pointer raw */
void print_raw( int len, char *buf )
{
	int i;
	for ( i = 0; i < len; i++ )
		printf( "%02x ", (unsigned char) buf[i] );
	printf( "\n" );
}

/* check whether a string does only contain letters, digits or
 * underscores */
int is_alphanum( char *str )
{
    int i;
    for ( i = 0; i< strlen(str); i++ )
        if ( !((str[i]>=48&&str[i]<=57)||(str[i]>=65&&str[i]<=90)||(str[i]>=97&&str[i]<=122)||str[i]=='_') )
            return 0;
    return 1;
}
