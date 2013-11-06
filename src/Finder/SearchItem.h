#ifndef SEARCH_ITEM_H
#define SEARCH_ITEM_H

#include <ListItem.h>
#include <string>

class SearchItem : public BStringItem
{
	private:
		string	m_file;
		float	m_weight;
		
	public:
		SearchItem( const char *, float );
		~SearchItem();
		
		const char * GetFile();
};

#endif
