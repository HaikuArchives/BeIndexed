#ifndef FINDER_H
#define FINDER_H

#include <Messenger.h>
#include <string>
#include <map>
#include <list>

enum {
	FINDER_RESULTS = 'FiRe',
	FINDER_WORDS = 'FiWo'
};

void search( map<string,double> & matches, list<string> & words );
void background_search( BMessenger, string query );

#endif
