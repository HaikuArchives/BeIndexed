#include "SearchItem.h"

#include <stdio.h>

SearchItem::SearchItem( const char * file, float weight )
:	BStringItem(""),
	m_file(file),
	m_weight(weight)
{
	char text[512];
	
	sprintf(text, "%.8f -- %s", m_weight, m_file.c_str() );
	
	SetText(text);
}

SearchItem::~SearchItem()
{
}

const char *
SearchItem::GetFile()
{
	return m_file.c_str();
}
