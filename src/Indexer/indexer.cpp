#include <iostream>
#include <fstream>
#include <string.h>
#include <string>
#include <list>
#include <map>
#include <File.h>
#include <stdio.h>
#include <Directory.h>
#include <Path.h>
#include <NodeInfo.h>
#include <time.h>
#include <vector>
#include <algorithm>
#include <strstream>

#include "utils.h"

#define DEBUG 

#define RELATIVE_LIMIT 0.25
#define DATABASE_MAINTENANCE_FREQUENCY 400

/**
	Index a text/plain-file.
*/
void 
text_plain_index_file( const char * filename, map<string,float> & wordlist )
{
	ifstream i(filename);
	
	string word;
	
	map<string,float>::iterator iter;
	
	float weight = 1.0;
	
	while ( !i.eof() )
	{
		skip_non_text(i);
		word = toLower( get_word(i) );
		
		if ( word == "" )
			// we get this at eof
			continue;
		
		if ( !is_word_valid( word ) )
			continue;
		
		iter = wordlist.find( word );
		if ( iter != wordlist.end() )
		{
			if ( wordlist[word] + weight < 1.0 )
				wordlist[word] = wordlist[word] + weight;
			else
				wordlist[word] = 1.0;
		} else 
		{
			wordlist[word] = weight;
		}
		
		if ( weight > 0.01 )
			weight -= 0.01;
	}
}


/**
	Get the MIME-type for a given file.
	
	Will eventually throw an error if the operation fails
*/
string
get_file_type( const char * filename )
{
	BEntry entry(filename);
	BNode node(&entry);
	BNodeInfo info(&node);
		
	char type[256];
	
	if ( info.GetType(type) != B_OK )
		// throw error here
		return "";
	else
		return type;
}

/**
	Defines a type<->handler relationship, used by init_type_handlers() and add-ons.
*/
typedef struct type_handler_t
{
	string type;
	void (*handler)(const char *, map<string,float> &);
};

/**
	Vector containing a list of current type handlers, use init_type_handlers() to
	set it up before calling index_file()
*/
vector<type_handler_t> g_type_handlers;

/**
	Initialize g_type_handlers, must be called before any call to index_file(), but
	must only be called once!
	
	Will eventually load add-ons and add them to the list as well to provide an easy
	way to add support for more formats.
*/
void
init_type_handlers()
{
	g_type_handlers.push_back( (type_handler_t){"text/plain",text_plain_index_file} );
	g_type_handlers.push_back( (type_handler_t){"text/x-source-code",text_plain_index_file} );
}


/**
	Check the file's type and call the corresponding handler.
	Will eventually throw an error if no handler is found, or an error occurs.
	Don't forget to call init_type_handlers() once at the start of the program
	to set up type > handler relationships (and load any present add-ons)
*/
void
index_file( const char * filename, map<string,float> & wordlist )
{
	string type = get_file_type(filename);
	
	for ( vector<type_handler_t>::iterator i = g_type_handlers.begin(); i != g_type_handlers.end(); i++ )
	{
		if ( (*i).type == type )
		{
			(*i).handler(filename,wordlist);
			return;
		}
	}
}

/**
	Query the database for the fileTable.id corresponding to path
*/
int
get_file_id( string path )
{
	string query = "SELECT id FROM fileTable WHERE path='";
	query += path;
	query += "';";
	g_sql->Exec( query );
	
	if ( g_sql->NumRows() == 0 )
		return -1;
	
	return atoi( g_sql->GetValue(0,0).c_str() );
}

/**
	Query the database for the wordTable.id corresponding to word
*/
int
get_word_id( string word )
{
	string query = "SELECT id FROM wordTable WHERE word='";
	query += word;
	query += "';";
	g_sql->Exec( query );
	
	if ( g_sql->NumRows() == 0 )
		return -1;
	
	return atoi( g_sql->GetValue(0,0).c_str() );
}

/**
	Checks if a handler that can handle the file's type is present.
	
	@param path Path of file to be examined
*/
bool
is_handler_present( const char * path )
{
	string type = get_file_type(path);
	
	if ( type == "" )
		return false;
	
	for ( vector<type_handler_t>::iterator i = g_type_handlers.begin(); i != g_type_handlers.end(); i++ )
	{
		if ( (*i).type == type )
		{
			return true;
		}
	}
	
	return false;
}

/**
	Checks if a file should be ignored and not included in the database.
	This is accomplished by checking a number of things:
	
		* If an attribute, BeIndexed:Rule, is set to 'ignore' or not.
		--- The rest are only checked if just_attr is false
		* If the file is a symlink
		* If there is an appropriate handler (unless check_handler is false)
	
	@param just_attr Determines if other checks than the attribute one should be made, default false
*/
bool 
should_ignore( const char * path, bool just_attr=false )
{
	BEntry entry(path);
	BNode node(&entry);
	
	char rule[256];
	
	ssize_t result = node.ReadAttr( "BeIndexed:Rule", B_STRING_TYPE, 0, rule, sizeof(rule) );
	
	if ( result > 0 )
	{ // attribute present, check it!
		rule[result] = 0;
		
		if ( strcmp(rule,"ignore") == 0 )
		{
			//#ifdef DEBUG
			cout << "BeIndexed:Rule = ignore: " << path << endl;
			//#endif
			return true;
		}
	}
	
	if ( just_attr )
		return false;
	
	if ( entry.IsSymLink() )
		return true;
	
	if ( !is_handler_present(path) )
		return true;
	
	if ( strstr(path, "/.svn/") )
		return true;
	
	return false;
}

/**
	Make sure that all words in list are present in wordTable
	
	@param wordlist List of words (and their weights) to be added to database
*/
void
add_words_to_wordtable( map<string,float> & wordlist )
{
	string query;
	
	map<string,float>::iterator iter = wordlist.begin();
	
	while ( iter != wordlist.end() )
	{
		int id = get_word_id(iter->first);
		
		if ( id < 0 )
		{ // add word
			query = "INSERT INTO wordTable (word) VALUES ('";
			query += iter->first;
			query += "')";
			
			if ( g_sql->Exec(query) != B_OK )
			{	// something went bad. return! maybe throw an error?
				return;
			}
		}
		
		iter++;
	}
}

/**
	Get the filesystem modification date for a file
*/
string
get_modification_date( const char * path )
{
	time_t	mod_time;
		
	BEntry entry(path);
		
	entry.GetModificationTime(&mod_time);
		
	tm *	mod_tm;
	mod_tm = localtime(&mod_time);
		
	// show what we're working with..
	char mod_date_string[100];
	sprintf(mod_date_string, "%d-%.02d-%.02d %.02d:%.02d:%.02d",
		mod_tm->tm_year + 1900,
		mod_tm->tm_mon + 1,
		mod_tm->tm_mday,
		mod_tm->tm_hour,
		mod_tm->tm_min,
		mod_tm->tm_sec
	);
	
	return mod_date_string;
}

/**
	Get the current date and time as a string
*/
string
get_current_date()
{
	time_t	mod_time = time(NULL);
	
	tm *	mod_tm;
	mod_tm = localtime(&mod_time);
		
	// show what we're working with..
	char mod_date_string[100];
	sprintf(mod_date_string, "%d-%.02d-%.02d %.02d:%.02d:%.02d",
		mod_tm->tm_year + 1900,
		mod_tm->tm_mon + 1,
		mod_tm->tm_mday,
		mod_tm->tm_hour,
		mod_tm->tm_min,
		mod_tm->tm_sec
	);
	
	return mod_date_string;
}

/**
	Update the stop-word-list to include any words that appear in more than 40% of
	the files in the database. Also removes those words from the contentTable.
	
	@param relative_limit Defines how common a word can be before it's added to the stop list, 0.0-1.0 in percent of total number of files
*/
void
update_stop_words(float relative_limit)
{
	return; // We ignore this function for now..
	//
	//
	//
	//
	//
	//
	//

	// get number of files
	g_sql->Exec("SELECT count(id) FROM fileTable");
	
	int tot_num_files = atoi( g_sql->GetValue(0,0).c_str() );
	
	int num_file_limit = (int)(tot_num_files * relative_limit);
	
	// get word list
	g_sql->Exec("SELECT id FROM wordTable");
	
	list<string> word_list;
	
	for ( int i=0; i<g_sql->NumRows(); i++ )
	{
		word_list.push_back(g_sql->GetValue(i,0));
	}
	
	// for each word, check how common it is and remove as needed
	for ( list<string>::iterator i=word_list.begin(); i != word_list.end(); i++ )
	{
		g_sql->Exec( string("SELECT count(word) FROM contentTable WHERE word=") + *i );
		
		int num_files = atoi( g_sql->GetValue(0,0).c_str() );
		
		g_sql->Exec( string("SELECT word FROM wordTable WHERE id=") + *i );
		string word = g_sql->GetValue(0,0);
			
		if ( num_files > num_file_limit || !is_word_valid(word) )
		{ // too many files, remove!
			#ifdef DEBUG
			cout << "  Removing word '" << word << "'" << endl;
			#endif
			
			g_sql->Exec("BEGIN");
			
			// add stop word
			g_sql->Exec( string("INSERT INTO stopwordTable (word) VALUES ('") + word + string("')") );
			g_stop_words.push_back( word );
			
			// delete from content
			g_sql->Exec( string("DELETE FROM contentTable WHERE word=") + *i );
			
			// delete from wordtable
			g_sql->Exec( string("DELETE FROM wordTable WHERE id=") + *i );
			
			g_sql->Exec("COMMIT");
		}
	}
}

// forward declaration..
void add_dir_to_database( const char * );

/**
	Indexes a file and adds the result to the database.
	
	@param path Path to the file to add
*/
void
add_file_to_database( const char * path )
{
	BEntry entry(path);
	
	if ( entry.IsDirectory() )
	{ // ooh, it's a dir and not a file!
		add_dir_to_database( path );
		return;
	}
	
	// update mime info so we know what we're handling
	update_mime_info(path,false/* recursive */,true /* sync */,false /* force */);
	
	// see if this is a file we can and want to handle
	if ( should_ignore(path) )
		return;
	
	char number[64];
	
	map<string,float> wordlist;
	
//	#ifdef DEBUG
	// seem like we want to index this, carry on..
	cout << "indexing file '" << path << "'" << endl;
//	#endif
	
	
	// look for file id in fileTable
	int file_id = get_file_id(path);
	
	if ( file_id < 0 )
	{ // file not in fileTable, add it
		string query = "INSERT INTO fileTable (path,indexed_at) VALUES ('"; // now
		query += path;
		query += "','1979-01-01 00:00:00');";
		g_sql->Exec( query );
		
		file_id = get_file_id(path);
	} else
	{ // check if file has been modified since it was indexed
		// get indexed_at date from database
		sprintf(number,"%d",file_id);
		string query = string("SELECT * FROM fileTable WHERE id=") + string(number);
		
		g_sql->Exec( query );
		
		string index_date = g_sql->GetValue(0,"indexed_at");
		index_date.resize(19);
		
		// get modification date from filesystem
		string fs_mod_date = get_modification_date(path);
		
		#ifdef DEBUG
		cout << "  i: " << index_date << " vs m: " << fs_mod_date << endl;
		#endif
		
		if ( index_date >= fs_mod_date )
		{ // no need to index, already up to date
//			#ifdef DEBUG
			cout << "  no need to index" << endl;
//			#endif
			return;
		}
	}
	
	// analyze the database once in a while to speed things up
	static int added_files = 0;
	
	if ( added_files++ % DATABASE_MAINTENANCE_FREQUENCY == 0 )
	{
		#ifdef USE_POSTGRES
		cout << "Analyzing database.." << endl;
		g_sql->Exec("ANALYZE;");
		#endif
		cout << "Updating stop word list..." << endl;
		update_stop_words(RELATIVE_LIMIT);
	}
	
	g_sql->Exec("BEGIN");
	
	// index file
	index_file(path,wordlist);
	
	#ifdef DEBUG
	cout << "  found " << wordlist.size() << " words" << endl;
	#endif
	
	// add words to wordtable
	string query;
	
	add_words_to_wordtable(wordlist);
	
	g_sql->Exec("END");
	
	// begin update
	
	g_sql->Exec("BEGIN");
		
		// delete old words
		sprintf(number,"%d",file_id);
		query = "DELETE FROM contentTable WHERE file=";
		query += number;
		query += ";";
		
		g_sql->Exec( query );
		
		// add new content
		for ( map<string,float>::iterator iter=wordlist.begin(); iter != wordlist.end(); iter++ )
		{
			query = "INSERT INTO contentTable (word,weight,file) VALUES ('";
			sprintf(number,"%d",get_word_id(iter->first));
			query += number;
			query += "',";
			sprintf(number,"%f",iter->second);
			query += number;
			query += ",";
			sprintf(number,"%d",file_id);
			query += number;
			query += ");";
			
			if ( g_sql->Exec(query) != B_OK )
			{ // error adding, do an ABORT and return
				g_sql->Exec("ROLLBACK");
				// actually we should throw an error, but..
				return;
			}
		}
	
	sprintf(number,"%d",file_id);
	query = "UPDATE fileTable SET indexed_at='" + get_current_date() + "' WHERE id=" + string(number);
	
	g_sql->Exec(query);
	
	g_sql->Exec("COMMIT");
}

/**
	Iterates through the files in a directory, adding them all to the database.
*/
void
add_dir_to_database( const char * path )
{
	BDirectory dir(path);
	BEntry entry;
	BPath p;
	
	if ( should_ignore(path,true /* only check the attribut since it's a dir*/ ) )
		return;
	
	while ( dir.GetNextEntry(&entry) == B_OK )
	{
		entry.GetPath(&p);
		
		add_file_to_database(p.Path());
	}
}

/**
	For each file: make sure that the file still exists in the filesystem, and if
	it doesn't remove it from the database.
*/
void
purge_deleted_files()
{
	g_sql->Exec("SELECT * FROM fileTable");
	
	// a list of filetable.id to delete
	list<string> deleted_files;
	
	for ( int i=0; i<g_sql->NumRows(); i++ )
	{
		string path = g_sql->GetValue(i,"path");
		
		BEntry entry( path.c_str() );
		
		if ( entry.Exists() )
			continue;
		
		cout << "Adding file to remove-list: " << path << endl;
		
		// file deleted, mark for deletion
		deleted_files.push_back( g_sql->GetValue(i,"id") );
	}
	
	list<string>::iterator i;
	
	g_sql->Exec("BEGIN");

	for ( i=deleted_files.begin(); i != deleted_files.end(); i++ )
	{
		cout << "Removing file id " << *i << endl;
		
		char query[512];
		
		{ // delete contentTable references
			sprintf(query, "DELETE FROM contentTable WHERE file=%s", i->c_str());
			
			cout << query << endl;
			
			g_sql->Exec( query );
		}
		{ // delete fileTable reference
			sprintf(query, "DELETE FROM fileTable WHERE id=%s", i->c_str());
		
			cout << query << endl;
			
			g_sql->Exec( query );
		}
	}
	
	g_sql->Exec("COMMIT");
}


/**
	Plain ol' main(). Performs some init, then adds something to the database
*/
int 
main(int numArg, char ** argv)
{
	set_thread_priority( find_thread(NULL), B_LOW_PRIORITY );
	
	system("mkdir -p `finddir B_USER_NONPACKAGED_DATA_DIRECTORY`/BeIndexed");
	
	init_sql();
	init_stop_words();
	init_type_handlers();
	
	if ( numArg < 2 )
	{
		cout << "usage: Indexer [<file or directory to index> || --update_stop_words]" << endl;
	} else {
		if ( string(argv[1]) == "--update_stop_words" )
		{
			update_stop_words(RELATIVE_LIMIT);
		} else 
		if ( string(argv[1]) == "--remove-deleted" )
		{
			purge_deleted_files();
		} else 
		{
			cout << "Removing deleted files from index..." << endl;
			purge_deleted_files();
		
			add_file_to_database(argv[1]);
		}
	}
	
	return 0;
}
