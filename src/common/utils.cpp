#include "utils.h"

#include <algorithm>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>

string
toLower(string str)
{
	for ( string::iterator i = str.begin(); i != str.end(); i++ )
	{
		if ( *i >= 'A' && *i <= 'Z' )
			*i = *i + 32;
	}
	
	return str;
}

bool 
is_non_text( char c )
{
	const char * ws = " \t\n\r\"()+,-=*/\\[]#{}^`?~;:|<>$&%'";
	
	for ( int i=0; ws[i]; i++ )
		if ( ws[i] == c )
			return true;
	
	return false;
}

void 
skip_non_text( istream & i )
{
	while ( !i.eof() && is_non_text(i.peek()) )
		i.get();
}

const char * g_word_endings[] = {
	"ing",
	"ly",
	"able",
	"s",
	".",
	",",
	"!",
	"-",
	NULL
};

string
stem( string _word )
{
	BString word(_word.c_str());
	
	for ( int i=0; g_word_endings[i]; i++ )
	{
		if ( word.FindLast(g_word_endings[i]) == (int32)(word.Length() - strlen(g_word_endings[i])) )
			word.RemoveLast(g_word_endings[i]);
	}
	
	return string(word.String());
}

string
strip_specials( string word )
{
	BString str(word.c_str());
//	str.RemoveAll("_");
//	str.RemoveAll("-");
	
	return string(str.String());
}

string
remove_duplicates( string word )
{
	word.erase (
		unique(word.begin(),
		word.end()),
		word.end()
	);
	
	return word;
}

string 
get_word( istream & i )
{
	string word;
	
	while ( !i.eof() && !is_non_text(i.peek()) )
	{
		word += i.get();
	}
	
	// lowercase
	transform(
		word.begin(), word.end(),
		word.begin(),
		tolower
	);
	
	return stem( strip_specials( remove_duplicates(word) ) );
}

bool
is_word_valid( string word )
{
	if ( word.length() < MIN_WORD_LENGTH || word.length() > MAX_WORD_LENGTH )
		// skip too small and too large words
		return false;
	
	if ( is_number(word) )
		// skip numbers
		return false;
	
	// skip if the entire word consists of a single char
	bool single_char = true;
	for ( unsigned int i=1; i<word.length(); i++ )
		if ( word[i] != word[0] )
		{
			single_char = false;
			break;
		}
	if ( single_char )
		return false;
	
	if ( is_stop_word(word) )
		// skip stop words
		return false;
	
	if ( word[0] == '0' && word[1] == 'x' )
	{ // 0x...
		return false;
	}
	
	return true;
}

bool
is_number( string word )
{
	// ignore ., since they're removed in get_word()
	
	for ( unsigned int i=0; i<word.length(); i++ )
	{
		if ( word[i] < '0' || word[i] > '9' )
			return false;
	}
	
	return true;
}

/**
	A list of 'stop words', populated by init_stop_words()
*/
vector<string>	g_stop_words;

const char * _stop_words[] = {
        "after",
        "although",
        "and",
        "as",
        "because",
        "before",
        "both",
        "but",
        "even",
        "for",
        "if",
        "like",
        "nor",
        "or",
        "since",
        "so",
        "than",
        "that",
        "then",
        "though",
        "unless",
        "until",
        "when",
        "where",
        "while",
        "yet",
        ""
};

/**
	Set up g_stop_words, a list of 'stop words'.
	
	Will eventually get the list of words to ignore from the database so the
	user can add or remove words as needed.
*/
void
init_stop_words()
{
	for ( int i=0; string(_stop_words[i]) != ""; i++ )
	{
		g_stop_words.push_back( string(_stop_words[i]) );
	}
/*
	g_sql->Exec("SELECT word FROM stopwordTable");
	
	for ( int i=0; i<g_sql->NumRows(); i++ )
	{
		g_stop_words.push_back( g_sql->GetValue(i,0) );
	}
*/
}

/**
	Determine if string is a 'stop word', a very common word that it makes no sense
	to include in the database, like 'the' and 'or'.
	
	init_stop_words() must be called before using this function
*/
bool
is_stop_word( string word )
{
	return find( g_stop_words.begin(), g_stop_words.end(), word ) != g_stop_words.end();
}

SQLConnection * g_sql=NULL;

#ifdef USE_SQLITE
void init_sql()
{
	BPath path;
	find_directory(B_USER_NONPACKAGED_DATA_DIRECTORY, &path);
	path.Append("BeIndexed");
	path.Append("BeIndexed.db");
	g_sql = new SQLiteConnection(path.Path());
	// set up tables. This shouldn't harm already existing tables.
	// wordtable
	g_sql->Exec("CREATE TABLE wordTable (id integer primary key, word text)");
	g_sql->Exec("CREATE INDEX wordtable_word_index ON wordTable (word)");
	g_sql->Exec("CREATE INDEX wordtable_id_index ON wordTable (id)");
	// filetable
	g_sql->Exec("CREATE TABLE fileTable (id integer primary key, path text, indexed_at datetime)");
	g_sql->Exec("CREATE INDEX filetable_path_index ON fileTable (path)");
	g_sql->Exec("CREATE INDEX filetable_id_index ON fileTable (id)");
	// contenttable
	g_sql->Exec("CREATE TABLE contentTable (word integer, file integer, weight float)");
	g_sql->Exec("CREATE INDEX content_word_index ON contentTable (word)");
	g_sql->Exec("CREATE INDEX content_file_index ON contentTable (file)");
	// stop word table
	g_sql->Exec("CREATE TABLE stopwordTable (word text)");
}
#endif // END OF USE_SQLITE

