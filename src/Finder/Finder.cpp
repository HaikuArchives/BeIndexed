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
#include <algorithm>
#include <vector>
#include <StopWatch.h>
#include <Application.h>
#include "FinderWindow.h"
#include <Message.h>
#include <strstream>

#include "utils.h"

#include "Finder.h"

#define DEBUG 

void
find_word( map<string,float> & results, string word, float weight )
{
	#ifdef DEBUG
	cout << "Searching for " << word << endl;
	#endif
	
	BStopWatch timer("word search");
	// TODO change this to ... WHERE word LIKE '%word%'
	string query = string("SELECT file,weight FROM contentTable WHERE word IN ") + 
		string("(SELECT id FROM wordTable WHERE word='") + word + string("')  ORDER BY weight DESC");
	g_sql->Exec( query );

	// store temporarily
	map<string, float> matches;
	
	for ( int i=0; i<g_sql->NumRows(); i++ )
	{
		matches[g_sql->GetValue(i,"file")] = atof(g_sql->GetValue(i,"weight").c_str()) * weight;
	}
	
	#ifdef DEBUG
	cout << "   Found " << g_sql->NumRows() << " matching files." << endl;
	#endif
	
	map<string,float>::iterator iter;
	for ( iter=matches.begin(); iter!=matches.end(); iter++ )
	{
		query = 
			string("SELECT path FROM fileTable WHERE id=" + iter->first);
		
		g_sql->Exec(query);
		
		results[g_sql->GetValue(0,"path")] = iter->second;
	}
}

void
find_subwords( map<string,float> & matches, string word )
{
	string query =
		string("SELECT word FROM wordTable where word like \"%" + word + "%\"");
	
	g_sql->Exec(query);
	
	list<string> words;
	
	for (int i=0; i<g_sql->NumRows(); i++ )
	{
		words.push_back( g_sql->GetValue(i,0) );
	}
	
	list<string>::iterator iter;
	
	for ( iter=words.begin(); iter!=words.end(); iter++ )
	{
		float weight = *iter == word ? 1.0 : 0.3;
		find_word(matches, *iter, weight);
	}
}

void
find_words( map<string,float> & matches, list<string> & words )
{
	BStopWatch timer("database search");
	
	list<string>::iterator curr = words.begin();
	
	#ifdef DEBUG
	cout << "Words: ";
	for ( ;curr != words.end(); curr++ )
		cout << *curr << " ";
	cout << endl;
	curr = words.begin();
	#endif
	
	if ( curr == words.end() )
		return;
	
	find_subwords(matches,*curr);
	
	curr++;
	
	// search for all words, but stop if we have no more matches
	while ( curr != words.end() && matches.size() > 0 )
	{
		map<string,float> results;
		
		find_subwords(results,*curr);
		
		// now, remove paths in matches that aren't in results
		// and update weight if they are
		map<string,float>::iterator iter = matches.begin();
		
		while ( iter != matches.end() )
		{
			if ( results.find(iter->first) == results.end() )
			{
				// not a match?
				#ifdef DEBUG
				cout << "removing " << iter->first << endl;
				#endif
				matches.erase(iter++);
			} else 
			{
				matches[iter->first] = matches[iter->first] + iter->second;
				iter++;
			}
		}
		
		curr++;
	}
}

typedef struct bg_search_data_t {
	BMessenger	msgr;
	string		query;
};

void
search( bg_search_data_t * data, map<string,float> & matches, list<string> & words )
{
	// stem words
	list<string> stemmed_words;
	
	list<string>::iterator curr;
	
	for ( curr = words.begin(); curr != words.end(); curr++ )
	{
		stemmed_words.push_back( stem(toLower(*curr)) );
	}
	
	// send a message to Finder with the stemmed words
	BMessage msg(FINDER_WORDS);
	for (curr = stemmed_words.begin(); curr != stemmed_words.end(); curr++ )
	{
		msg.AddString("word", (*curr).c_str());
	}
	data->msgr.SendMessage(&msg);
	
	// perform search
	find_words(matches,stemmed_words);
}

int32
background_search_thread( void * _data )
{
	bg_search_data_t * data = (bg_search_data_t*)_data;
	
	// split query
	list<string> words;
	
	istrstream i( data->query.c_str() );
	
	while ( !i.eof() )
	{
		skip_non_text(i);
		string word = get_word(i);
		
		if ( word == "" )
			// we get this at eof
			continue;
		
		words.push_back(word);
	}
	
	// hackish bug fix: remove last character in last word. this shouldn't be needed
	string last_word = (*(--words.end()));
	words.pop_back();
	last_word.erase( last_word.length()-1, 1 );
	words.push_back( last_word );
	
	map<string,float> matches;
	
	// search
	if ( words.size() > 0 )
		search(data,matches,words);
	
	// sort matches by weight instead of by path
	vector<pair<float,string> > result;
	
	map<string,float>::iterator iter = matches.begin();
	
	while ( iter != matches.end() )
	{
		result.push_back( pair<float,string>(iter->second,iter->first) );
		iter++;
	}
	
	// we actually want to reverse_sort(being,end), but..
	sort(result.begin(),result.end());
	reverse(result.begin(),result.end());

	
	BMessage msg(FINDER_RESULTS),reply;
	
	// foreach match add to msg
	for ( vector<pair<float,string> >::iterator i = result.begin(); i != result.end(); i++ )
	{
		msg.AddString("path", (*i).second.c_str());
		msg.AddFloat("weight", (*i).first);
	}
	
	data->msgr.SendMessage(&msg,&reply);
	
	delete data;
	
	return 0;
}

void
background_search( BMessenger msgr, string query_string )
{
	bg_search_data_t * data = new bg_search_data_t;
	
	data->msgr = msgr;
	data->query = query_string;
	
	thread_id t = spawn_thread(
		background_search_thread,
		"Searcher",
		B_NORMAL_PRIORITY,
		data
	);
	
	resume_thread(t);
}


int 
main(int numArg, const char ** argv)
{
/*	list<string> words;
	
	for ( int i=1; i<numArg; i++ )
		words.push_back( toLower(argv[i]) );
*/
	init_sql();
	
	BApplication app("application/x-vnd.m_eiman.Finder");
	
	FinderWindow * win = new FinderWindow;
	win->Show();
	
	app.Run();
	
	return 0;
}
