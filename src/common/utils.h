#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <iostream>
#include <vector>

#include "SQLConnection.h"

string	toLower(string);

/**
	Determines if a character is a non-text character
	
	Will eventually be extended to support multi-byte characters
*/
bool	is_non_text( char c );

/**
	Read ahead in istream and skip any whitespace
*/
void	skip_non_text( istream & i );

/**
	Get the next word from an istream.
*/
string	get_word( istream & i );

/**
	Get the stem of the word
*/
string	stem( string _word );

/**
	Determine if string contains only digits
*/
bool	is_number( string word );

/**
	Check if a word is something that could/should be added
	to the database
*/
bool	is_word_valid( string word );

/**
	A list of 'stop words', populated by init_stop_words()
*/
extern vector<string> g_stop_words;

/**
	Determine if string is a 'stop word', a very common word that it makes no sense
	to include in the database, like 'the' and 'or'.
	
	init_stop_words() must be called before using this function
*/
bool	is_stop_word( string word );

/**
	Set up g_stop_words, a list of 'stop words'.
	
	Will eventually get the list of words to ignore from the database so the
	user can add or remove words as needed.
*/
void	init_stop_words();

/**
	Connection to a SQL source.
*/
extern SQLConnection * g_sql;

/**
	Inits the SQL connection. Run at start of app.
*/
void init_sql();

enum {
	MIN_WORD_LENGTH = 2,
	MAX_WORD_LENGTH = 25
};

#endif
