#ifndef FINDER_WINDOW_H
#define FINDER_WINDOW_H

#include "MWindow.h"
#include "MListView.h"
#include "MTextControl.h"
#include "MButton.h"
#include "MStringView.h"

#include <StopWatch.h>

/**
	GUI for searching the database.
*/
class FinderWindow : public MWindow
{
	private:
		MTextControl	* m_text;
		MListView		* m_list;
		MStringView		* m_status;
		BStopWatch		m_timer;
		
		enum {
			SEARCH_BUTTON 	= 'SeBu',
			LIST			= 'List'
		};
		
	public:
		FinderWindow();
		virtual ~FinderWindow();
		
		virtual bool QuitRequested();
		
		virtual void MessageReceived( BMessage * );
};

#endif
