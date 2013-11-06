#include "FinderWindow.h"

#include <Application.h>

#include "HGroup.h"
#include "VGroup.h"
#include "MBorder.h"
#include "MScrollView.h"

#include "Finder.h"
#include "SearchItem.h"

#include <ListItem.h>
#include <Entry.h>
#include <strstream>

FinderWindow::FinderWindow()
:	MWindow(
		BRect(100,100,600,400),
		"Finder",
		B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS|B_QUIT_ON_WINDOW_CLOSE
	),
	m_timer("Database search timer")
{
	BView * view =
	new VGroup(
		new MBorder(
			B_FANCY_BORDER,
			5,
			NULL,
			new VGroup(
				m_text = new MTextControl(
					"Search", ""
				),
				m_status = new MStringView(
					"Enter search terms and press Enter to search."
				),
				NULL
			)
		),
		new MScrollView(
			m_list = new MListView(),
			false,
			true
		),
		NULL
	);
	
	AddChild( view );
	
	m_text->MakeFocus();
	m_text->SetMessage( new BMessage(SEARCH_BUTTON) );
	m_list->SetMessage( new BMessage(LIST) );
}

FinderWindow::~FinderWindow()
{
}

bool
FinderWindow::QuitRequested()
{
	BMessenger(be_app).SendMessage( B_QUIT_REQUESTED );
	return true;
}

void
FinderWindow::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
		case LIST:
		{
			int32 offset=-1;
			if ( msg->FindInt32("index",&offset) == B_OK )
			{
				SearchItem * item = (SearchItem*)m_list->ItemAt(offset);
				entry_ref ref;
				get_ref_for_path(item->GetFile(),&ref);
				
				BMessage message(B_REFS_RECEIVED);
				message.AddRef("refs", &ref);
				BMessenger trackerMsgr( "application/x-vnd.Be-TRAK" );
				trackerMsgr.SendMessage(&message);
			}
		}	break;
		
		case SEARCH_BUTTON:
			m_timer.Reset();
			m_status->SetText("Searching...");
			background_search( BMessenger(this), m_text->Text() );
			break;
		
		case FINDER_RESULTS:
		{
			// clear list
			while ( m_list->ItemAt(0) )
			{
				BListItem * item = m_list->ItemAt(0);
				m_list->RemoveItem(item);
				delete item;
			}
			
			// add new results
			int count=0;
			const char * path;
			for ( count=0; msg->FindString("path",count,&path) == B_OK; count++ )
			{
				float weight=0.0;
				msg->FindFloat("weight",count,&weight);
				
				m_list->AddItem( new SearchItem(path,weight) );
			}
			
			strstream str;
			str.precision(2);
			str << "Found " << count << " matching files in " << m_timer.ElapsedTime() / 1000000.0 << "s. "
				<< "Enter new search terms and press Enter to search again.";
			
			char status[1024];
			str.getline(status,sizeof(status)-1);
				
			m_status->SetText( status );
		}	break;
		
		case FINDER_WORDS:
		{
			string text("Search word(s):");
			
			for ( int i=0; msg->FindString("word",i); i++ )
			{
				text += " ";
				text += msg->FindString("word",i);
			}
			
			m_status->SetText(text.c_str());
		}	break;
		
		default:
			MWindow::MessageReceived(msg);
	}
}
