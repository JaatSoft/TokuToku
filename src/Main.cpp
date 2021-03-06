/*
	Main.cpp
	Code: aljen <aljen@gumisie.be>
	Homepage: http://begadu.sf.net
*/

#include <Alert.h>
#include <Application.h>
#include <Beep.h>
#include <Bitmap.h>
#include <DataIO.h>
#include <FindDirectory.h>
#include <ListView.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <Point.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>
#include <LayoutBuilder.h>
#include <Notification.h>
#include <stdlib.h>
#include <string.h>

#include "GaduListItem.h"
#include "GaduMenuItem.h"
#include "GfxStuff.h"
#include "Main.h"
#include "Msg.h"
#include "Network.h"
#include "Preferences.h"
#include "Person.h"
#include "ContactList.h"
#include "StatusWindow.h"


static const char *
string_from_status(int32 status)
{
	switch (status) {
		case GG_STATUS_NOT_AVAIL:
		case GG_STATUS_NOT_AVAIL_DESCR:
			return "Niedostępny";

		case GG_STATUS_AVAIL:
		case GG_STATUS_AVAIL_DESCR:
			return "Dostępny";

		case GG_STATUS_BUSY:
		case GG_STATUS_BUSY_DESCR:
			return "Zajęty";

		case GG_STATUS_INVISIBLE:
		case GG_STATUS_INVISIBLE_DESCR:
			return "Niewidoczny";

		case GG_STATUS_BLOCKED:
			return "Zablokowany";

		default:
			break;
	}

	return "Nieznany";
}

static bool
status_added_descr(int32 olds, int32 news)
{
	if ((olds == GG_STATUS_NOT_AVAIL && news == GG_STATUS_NOT_AVAIL_DESCR) ||
		(olds == GG_STATUS_AVAIL && news == GG_STATUS_AVAIL_DESCR) ||
		(olds == GG_STATUS_BUSY && news == GG_STATUS_BUSY_DESCR) ||
		(olds == GG_STATUS_INVISIBLE && news == GG_STATUS_INVISIBLE_DESCR))
		return true;
	return false;
}


#define MAINWINDOW_RECT BRect(50,50,300,350)
#define MAINWINDOW_NAME	"TokuToku " WERSJA

MainWindow::MainWindow( Profile* aProfile )
 	: BWindow( MAINWINDOW_RECT, MAINWINDOW_NAME, B_TITLED_WINDOW, 
 			   B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS )
	{
	//iProfile = new Profile();
	//iProfile->Load( aProfile );
	fprintf(stderr, "be");
	iProfile = aProfile;
	//SetTitle( aProfile->String() );
	SetTitle("Test");
	BRoster roster;
	entry_ref ref;
	BFile resfile;
	roster.FindApp( "application/x-vnd.TokuToku", &ref );
	resfile.SetTo( &ref, B_READ_ONLY );
	iResources.SetTo( &resfile );
	LoadIcons();
	SetSizeLimits( 250, 600, 300, 600 );
	iNetwork = new Network( iProfile, iProfile->GetUserlist()->GetList());
	iNetwork->GotWindow( this );
	iListItems = new List( 512 );
	if( iProfile->iRect.IsValid() )
		{
		MoveTo( iProfile->iRect.left, iProfile->iRect.top );
		}
	/* setting menu */
	BRect r = Bounds();
	BMenuBar *menuBar = new BMenuBar( r, "menu bar" );
	BMenu *menu;
	menu = new BMenu( "Menu" );

	iSubMenu = new BMenu( "Lista kontaktów" );
	iListImport = new BMenuItem( "Importuj", new BMessage( BEGG_IMPORT_LIST ) );
	iListExport = new BMenuItem( "Eksportuj", new BMessage() );
	iSubMenu->AddItem( iListImport );
	iSubMenu->AddItem( iListExport );
	iListMenu = new BMenuItem( iSubMenu, NULL );
	menu->AddItem( iListMenu );
	menu->AddSeparatorItem();

	iPreferences = new BMenuItem( "Ustawienia", new BMessage( BEGG_PREFERENCES ) );
	menu->AddItem( iPreferences );
	iAbout = new BMenuItem( "O TokuToku..", new BMessage( BEGG_ABOUT ) );
	menu->AddItem( iAbout );
	menu->AddItem( new BMenuItem( "Zakończ", new BMessage( BEGG_QUIT ) ) );
	menuBar->AddItem( menu );
	AddChild( menuBar );

	r.top = menuBar->Bounds().bottom ;
//	r.bottom = r.top + 32;
//	fIconsView = new BView( BRect(r.left, r.top, r.right, r.bottom), "IconsView", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP , B_FULL_UPDATE_ON_RESIZE);
//	fIconsView->SetViewColor(100,100,100);
//	AddChild(fIconsView);

//	BBitmap *stan1 = LoadGFX("/boot/apps/TokuToku/images/1.png");
//	BBitmap *stan2 = LoadGFX("/boot/apps/TokuToku/images/2.png");
//	IconControl *iconcontrol = new IconControl(BRect(r.left, r.top + 3, r.left + 30, r.top + 30), "iconcontrol", stan1, stan2, new BMessage(B_QUIT_REQUESTED));
//	AddChild(iconcontrol);
	
//	r = Bounds();
//	r.top = fIconsView->Bounds().bottom + menuBar->Bounds().bottom;
	
	iGaduView = new BView( r, "iGaduView", B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_SUBPIXEL_PRECISE );
	iGaduView->SetViewColor( 90, 90, 90 );
	AddChild( iGaduView );
	
	/* list */
	r = iGaduView->Bounds();
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom -= 30;

	iListView = new ContactList( r, "iListView", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE  );
	iListView->SetViewColor( 110, 110, 110 );
	BFont *font = new BFont( be_plain_font );
	font->SetSize( 15.0 );
	font->SetEncoding( B_ISO_8859_2 );
	iListView->SetFont( font );
	iScrollView = new BScrollView( "iScrollView", iListView, B_FOLLOW_ALL, B_SUBPIXEL_PRECISE | B_FRAME_EVENTS | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE, false, true, B_PLAIN_BORDER );
	iGaduView->AddChild( iScrollView );
	
	/* we must know if user select/open person */
	iListView->SetSelectionMessage( new BMessage( BEGG_PERSON_SELECTED ) );
	iListView->SetInvocationMessage( new BMessage( BEGG_PERSON_ACTION ) );

	/* adding status */
	r = iGaduView->Bounds();
	r.top = iListView->Bounds().bottom + 5;
//	r.top = iGaduView->Bounds().bottom;
	r.left = r.left + 5;
	r.right = r.right -5;

	BMenuItem *selectstatus;
	iStatusMenu = new BPopUpMenu( "change_status" );
	iAvail			= new GaduMenuItem( iIconAvail, "Dostępny", new BMessage( SET_AVAIL ) );
	iBRB			= new GaduMenuItem( iIconBRB, "Zaraz wracam", new BMessage( SET_BRB ) );
	iInvisible		= new GaduMenuItem( iIconInvisible, "Niewidoczny", new BMessage( SET_INVIS ) );
	iNotAvail		= new GaduMenuItem( iIconNotAvail, "Niedostępny", new BMessage( SET_NOT_AVAIL ) );
	iDescr			= new GaduMenuItem( iIconAvailDescr, "Z Opisem", new BMessage( SET_DESCRIPTION ) );	
	
	iStatusMenu->AddItem( iAvail );
	iStatusMenu->AddItem( iBRB );
	iStatusMenu->AddItem( iInvisible );
	iStatusMenu->AddItem( iNotAvail );
	iStatusMenu->AddItem( iDescr );
	iNotAvail->SetMarked( true );
	
	iStatus = new BMenuField( r, "iStatus", "Status:", iStatusMenu, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS );
	iGaduView->AddChild( iStatus );

	add_system_beep_event( ONLINE_SOUND );
	add_system_beep_event( MESSAGE_SOUND );
	
	if( iProfile->GetUserlist()->IsValid() )
		{
		iListItems = iProfile->GetUserlist()->GetList();
		BMessenger( this ).SendMessage( UPDATE_LIST );
		}

	fprintf( stderr,"Profile %s loaded.\n", iProfile->ProfileName()->String() );

	if( iProfile->AutoStatus() != GG_STATUS_NOT_AVAIL )
		{
		if( iNetwork->Session() )
			{
			gg_change_status( iNetwork->Session(), iProfile->AutoStatus() );
			}
		else
			{
			iNetwork->Login( iProfile->AutoStatus() );
			}
		}
	}

bool MainWindow::QuitRequested()
	{
	fprintf( stderr, "MainWindow::QuitRequested()\n" );
	/* saving profile settings */
	if( LockLooper() )
		{
		if( !IsHidden() )
			{
			Hide();
			}
		UnlockLooper();
		}
	return false;
	}

void MainWindow::MessageReceived( BMessage* aMessage )
	{
	switch( aMessage->what )
		{
		case BEGG_PREFERENCES:
			{
			fprintf( stderr, "MainWindow::MessageReceived( BEGG_PREFERENCES )\n" );
			BScreen *screen = new BScreen( this );
			display_mode mode;
			screen->GetMode( &mode );
			// centering
			int32 width = 600;
			int32 height = 400; 
			// obliczamy srodek ekranu /2  - 1/2 szerokosci okna
			int32 x_wind = mode.timing.h_display / 2 - ( width / 2);
			// obliczamy srodek ekranu /2 - 1/2 wysokosci okna
			int32 y_wind = mode.timing.v_display / 2 - ( height / 2 );
			int32 new_width = x_wind + width;	// x 2
			int32 new_height = y_wind + height;		// x 2
			Preferences* prefs = new Preferences( iProfile, this, BRect( x_wind, y_wind, 350, 450 ), &iResources );
			prefs->Show();
			break;
			}
		
		case BEGG_ABOUT:
			{
			fprintf( stderr, "MainWindow::MessageReceived( BEGG_ABOUT )\n" );
			AboutRequested();
			break;
			}
		
		case SHOW_MAIN_WINDOW:
			{
			fprintf( stderr, "MainWindow::MessageReceived( SHOW_MAIN_WINDOW )\n" );
			if( LockLooper() )
				{
				if( IsHidden() )
					{
					Show();
					}
				else
					{
					Activate();
					}
				UnlockLooper();
				}
			break;
			}
			
		case BEGG_PERSON_SELECTED:
			{
			fprintf( stderr, "MainWindow::MessageReceived( BEGG_PERSON_SELECTED )\n" );
			if( iListView->CurrentSelection() < 0 )
				{
				break;
				}
			Person *person = NULL;
			GaduListItem *who = ( GaduListItem* ) iListView->ItemAt( iListView->CurrentSelection() );
			for( int i = 0; i < iProfile->GetUserlist()->GetList()->CountItems(); i++ )
				{
				person = ( Person* ) iProfile->GetUserlist()->GetList()->ItemAt( i );
				if( !person->iDisplay->Compare( who->iName->String() ) )
					{
					break;
					}
				}
			break;
			}
			
		case BEGG_PERSON_ACTION:
		{
			fprintf( stderr, "MainWindow::MessageReceived( BEGG_PERSON_ACTION )\n" );
			Person *person = GetContactAt(iListView->CurrentSelection());
			if (person == NULL || person->iUIN == iProfile->iNumber)
				break;

			BMessage *message;
			message = new BMessage(OPEN_MESSAGE);
			message->AddInt32("who", person->iUIN);
			BMessenger(iNetwork).SendMessage(message);
			delete message;
			break;
		}

		case CONTACT_RIGHTCLICK:
		{
			fprintf(stderr, "MainWindow::MessageReceived(CONTACT_RIGHTCLICK)\n");
			BPoint where(0, 0);
			aMessage->FindPoint("where", &where);
			ShowContactMenu(where);
			break;
		}

		case SET_AVAIL:
			{
			fprintf( stderr, "MainWindow::MessageReceived( SET_AVAIL )\n" );
			if( iNetwork->Session() )
				{
				gg_change_status( iNetwork->Session(), GG_STATUS_AVAIL );
				iNetwork->SetStatus( GG_STATUS_AVAIL );
				BMessenger( this ).SendMessage( UPDATE_LIST );
				}
			else
				{
				iNetwork->Login( GG_STATUS_AVAIL );
				}
			iProfile->SetAutoStatus( GG_STATUS_AVAIL );
			BMessage* message = new BMessage( BGDESKBAR_CHSTATE );
			message->AddInt16( "iStatus", iNetwork->iStatus );
			BMessenger( this ).SendMessage( message );
			delete message;
			break;
			}
			
		case SET_BRB:
			{
			fprintf( stderr, "MainWindow::MessageReceived( SET_BRB )\n" );
			if( iNetwork->Session() )
				{
				gg_change_status( iNetwork->Session(), GG_STATUS_BUSY );
				iNetwork->SetStatus( GG_STATUS_BUSY );
				BMessenger( this ).SendMessage( UPDATE_LIST );
				}
			else
				{
				iNetwork->Login( GG_STATUS_BUSY );
				}
			iProfile->SetAutoStatus( GG_STATUS_BUSY );
			BMessage* message = new BMessage( BGDESKBAR_CHSTATE );
			message->AddInt16( "iStatus", iNetwork->iStatus );
			BMessenger( this ).SendMessage( message );
			delete message;
			break;
			}
			
		case SET_INVIS:
			{
			fprintf( stderr, "MainWindow::MessageReceived( SET_INVIS )\n" );
			if( iNetwork->Session() )
				{
				gg_change_status( iNetwork->Session(), GG_STATUS_INVISIBLE );
				iNetwork->SetStatus( GG_STATUS_INVISIBLE );
				BMessenger( this ).SendMessage( UPDATE_LIST );
				}
			else
				{
				iNetwork->Login( GG_STATUS_INVISIBLE );
				}
			iProfile->SetAutoStatus( GG_STATUS_INVISIBLE );
			BMessage* message = new BMessage( BGDESKBAR_CHSTATE );
			message->AddInt16( "iStatus", iNetwork->iStatus );
			BMessenger( this ).SendMessage( message );
			delete message;
			break;
			}
		
		case SET_NOT_AVAIL:
			{
			fprintf( stderr, "MainWindow::MessageReceived( SET_NOT_AVAIL )\n" );
			if( iNetwork->Session() )
				{
				iNetwork->Logout();
				}
			iProfile->SetAutoStatus( GG_STATUS_NOT_AVAIL );
			BMessage* message = new BMessage( BGDESKBAR_CHSTATE );
			message->AddInt16( "iStatus", iNetwork->iStatus );
			BMessenger( this ).SendMessage( message );
			delete message;
			break;
			}
			
		case UPDATE_STATUS:
			{
			fprintf( stderr, "MainWindow::MessageReceived( UPDATE_STATUS )\n" );
			switch( iNetwork->GetStatus() )
				{
				case GG_STATUS_NOT_AVAIL:
					{
					iNotAvail->SetMarked( true );
					break;
					}
					
				case GG_STATUS_INVISIBLE:
					{
					iInvisible->SetMarked( true );
					break;
					}
					
				case GG_STATUS_BUSY:
					{
					iBRB->SetMarked( true );
					break;
					}
					
				case GG_STATUS_AVAIL:
					{
					iAvail->SetMarked( true );
					break;
					}
					
				case GG_STATUS_AVAIL_DESCR:
				case GG_STATUS_BUSY_DESCR:
				case GG_STATUS_INVISIBLE_DESCR:
				case GG_STATUS_NOT_AVAIL_DESCR:
					{
					iDescr->SetMarked( true );
					break;			
					}
				}
			break;
			}
			
		case UPDATE_LIST:
			{
			fprintf( stderr, "MainWindow::MessageReceived( UPDATE_LIST )\n" );
			List *listAvail = new List( 512 );
			List *listNotAvail = new List( 512 );
			
			GaduListItem *g = NULL;
			Person	 *p = NULL;
			for( int i = 0; i < iProfile->GetUserlist()->GetList()->CountItems(); i++ )
				{
				p = ( Person* ) iProfile->GetUserlist()->GetList()->ItemAt( i );
				
				if( p->iUIN == iProfile->iNumber )
					{
					g = new GaduListItem( p->iDisplay, iNetwork->GetStatus(), iProfile->iDescription, &iResources, p );
					if( iNetwork->GetStatus() == GG_STATUS_NOT_AVAIL || iNetwork->GetStatus() == GG_STATUS_NOT_AVAIL_DESCR ||
						iNetwork->GetStatus() == GG_STATUS_INVISIBLE || iNetwork->GetStatus() == GG_STATUS_INVISIBLE_DESCR )
						{
						listNotAvail->AddItem( g );
						}
					else
						{
						listAvail->AddItem( g );
						}
					}
				else
					{
					if( iNetwork->GetStatus() == GG_STATUS_NOT_AVAIL || iNetwork->GetStatus() == GG_STATUS_NOT_AVAIL_DESCR )
						{
						BString *empty = new BString();
						empty->SetTo( "" );
						g = new GaduListItem( p->iDisplay, p->iStatus, empty, &iResources, p );
						}
					else
						{
						g = new GaduListItem( p->iDisplay, p->iStatus, p->iDescription, &iResources, p );
						}

					if( p->iStatus == GG_STATUS_NOT_AVAIL || p->iStatus == GG_STATUS_NOT_AVAIL_DESCR )
						{
						listNotAvail->AddItem( g );
						}
					else
						{
						listAvail->AddItem( g );
						}
					}
				}
			iListView->MakeEmpty();
			listAvail->SortItems( SortUsers );
			iListView->AddList( listAvail );
			listNotAvail->SortItems( SortUsers );
			iListView->AddList( listNotAvail );
			iListView->Invalidate();
			BScrollBar* sb = iScrollView->ScrollBar( B_VERTICAL );
			float listHeight = iScrollView->Bounds().bottom - iScrollView->Bounds().top;
			float itemsHeight = iListView->CountItems() * 35;
			float propH = listHeight / itemsHeight;
			float rangeH = itemsHeight - listHeight;
			sb->SetProportion( propH );
			sb->SetRange( 0, rangeH );
			sb->SetSteps( listHeight / 8.0, listHeight / 2.0 );
			delete listAvail;
			delete listNotAvail;
			break;
			}
			
		case BEGG_IMPORT_LIST:
			{
			fprintf( stderr, "MainWindow::MessageReceived( BEGG_IMPORT_LIST )\n" );
			if( iNetwork->Session() )
				{
				iProfile->GetUserlist()->Import( iNetwork->Session(), iProfile->GetUserlist()->GetList() );
				}
			else
				{
				BAlert *alert = new BAlert( "List", "Musisz być połączony by zaimportować listę", "Ok" );
				alert->Go();
				}
			break;
			}
		
		case BGDESKBAR_CHSTATE:
			{
			fprintf( stderr, "MainWindow::MessageReceived( BGDESKBAR_CHSTATE )\n" );
			iDeskbarMessenger.SendMessage( aMessage );
//			BMessenger deskbar( "application/x-vnd.Be-TSKB");
//			deskbar.SendMessage( aMessage );
//			BRoster roster;
//			roster.Broadcast( aMessage );
			break;
			}
		
		case BEGG_QUIT:
			{
			iProfile->SetRect( Frame() );
			iProfile->Save();
			/* cleaning up ;D */
			iNetwork->GotWindow( NULL );
			iNetwork->Quit();
			BMessenger( be_app ).SendMessage( B_QUIT_REQUESTED );
			break;
			}

		// TODO: Maybe move all these things to ContactList?
		case CONTACTMENU_INFO:
		{
			fprintf( stderr, "MainWindow::MessageReceived( CONTACTMENU_INFO )\n" );

			Person *target = GetContactAt(iListView->CurrentSelection());
			if (target == NULL)
				break;

			// XXX: Temporary until StatusWindow is not finished
			BString content("");
			content << "Numer GG: " << target->iUIN << "\n";
			content << "Opis:\n" << target->iDescription->String() << "\n";
			BAlert *info = new BAlert("contact_info",
				   content.String(),
				   "OK");
			info->SetShortcut(0, B_ESCAPE);
			if (info != NULL)
				info->Go();

			break;
		}

		case CONTACT_STATUS_CHANGED:
			fprintf( stderr, "MainWindow::MessageReceived( CONTACT_STATUS_CHANGED )\n" );
			if (iProfile->iStatusNotify) {
				BString s("");

				Person* o = NULL;
				int32 uin = 0;
				aMessage->FindInt32("UIN", &uin);
				if (!(o = iProfile->GetUserlist()->Find(uin)))
					break;
				int32 old = 0;
				aMessage->FindInt32("old_status", &old);
				if (old == o->iStatus || status_added_descr(old, o->iStatus))
					break;

				s << o->iDisplay->String() << " jest teraz " << 
					string_from_status(o->iStatus);

				BNotification notify(B_INFORMATION_NOTIFICATION);
				notify.SetGroup("TokuToku");
				notify.SetTitle("Kontakt zmienił stan");
				notify.SetContent(s.String());
				notify.SetOnClickApp("application/x-vnd.TokuToku");

				notify.Send((bigtime_t)0);
			}
			break;

		default:
			BWindow::MessageReceived( aMessage );
			break;
		}
	}

Person *
MainWindow::GetContactAt(int index)
{
	if (index < 0)
		return NULL;

	Person *person = NULL, *whoPerson = NULL;
	BList *list = iProfile->GetUserlist()->GetList();
	int i, count = list->CountItems();
	GaduListItem *who = (GaduListItem *)iListView->ItemAt(index);
	whoPerson = who->GetPerson();
	for (i = 0; i < count; i++) {
		person = (Person *)list->ItemAt(i);
		if (person->iUIN == whoPerson->iUIN)
			return person;
	}

	return NULL;
}


int MainWindow::SortUsers( const void *left, const void *right )
	{
//	fprintf( stderr, "MainWindow::SortUsers()\n" );
	const GaduListItem 	**firstItem( ( const GaduListItem ** ) left ),
						**secondItem( ( const GaduListItem ** ) right );
	BString users[ 2 ];
	
	users[ 0 ] = ( *firstItem )->iName->String();
	users[ 1 ] = ( *secondItem )->iName->String();
	return users[ 0 ].ICompare( users[ 1 ] );
	}

void MainWindow::LoadIcons()
	{
	fprintf( stderr, "MainWindow::LoadIcons()\n" );
	iIconAvail	 			= GetBitmap( "online" );
	iIconBRB		 		= GetBitmap( "busy" );
	iIconInvisible	 		= GetBitmap( "invisible" );
	iIconNotAvail	 		= GetBitmap( "offline" );
	iIconAvailDescr 		= GetBitmap( "online_d" );
	iIconBRBDescr		 	= GetBitmap( "busy_d" );
	iIconInvisibleDescr 	= GetBitmap( "invisible_d" );
	iIconNotAvailDescr	 	= GetBitmap( "offline_d" );
	}

void MainWindow::SetProfile( BString* aProfile )
	{
	fprintf( stderr, "MainWindow::SetProfil()\n" );
	iProfile->Load( aProfile );
	SetTitle( iProfile->ProfileName()->String() );
	if( iProfile->GetUserlist()->IsValid() )
		{
		iListItems = iProfile->GetUserlist()->GetList();
		BMessenger( this ).SendMessage( UPDATE_LIST );
		}
/*
	if( fProfil->fAutoStatus != GG_STATUS_NOT_AVAIL )
	{
		if(fSiec->fSesja)
			gg_change_status(fSiec->fSesja, fProfil->fAutoStatus);
		else
			fSiec->Login(fProfil->fAutoStatus);
	}
*/
	}

BBitmap *MainWindow::GetBitmap( const char* aName )
	{
	BBitmap 	*bitmap = NULL;
	size_t 		len = 0;
	status_t 	error;	

	const void *data = iResources.LoadResource( 'BBMP', aName, &len );

	BMemoryIO stream( data, len );
	
	BMessage archive;
	error = archive.Unflatten( &stream );
	if( error != B_OK )
		return NULL;
	bitmap = new BBitmap( &archive );
	if( !bitmap )
		return NULL;

	if( bitmap->InitCheck() != B_OK )
		{
		delete bitmap;
		return NULL;
		}
	
	return bitmap;
	}

Profile* MainWindow::GetProfile() const
	{
	return iProfile;
	}

Network* MainWindow::GetNetwork() const
	{
	return iNetwork;
	}

BListView* MainWindow::ListView() const
	{
	return iListView;
	}

GaduListItem* MainWindow::ListItem() const
	{
	return iListItem;
	}

List* MainWindow::ListItems() const
	{
	return iListItems;
	}

void MainWindow::SetMessenger( BMessenger& aMessenger )
	{
	iDeskbarMessenger = aMessenger;
	}

void MainWindow::AboutRequested(void)
{
	BAlert *about = new BAlert("about",
	       "TokuToku (TokuToku)\n\n"
	       "Autorzy:\n"
	       "     Artur Wyszynski (autor orginalu)\n"
	       "     Lukasz 'sil2100' Zemczak\n",
	       "OK");
	about->SetShortcut(0, B_ESCAPE);
	if (about != NULL)
		about->Go();
}

void
MainWindow::ShowContactMenu(BPoint where)
{
	BPopUpMenu *menu = new BPopUpMenu("contact_menu", false, false);

	BLayoutBuilder::Menu<>(menu)
		.AddItem("Informacje", CONTACTMENU_INFO)
		.AddItem("Rozmowa", BEGG_PERSON_ACTION)
		.AddItem("Dziennik rozmów", CONTACTMENU_LOGS)
			.SetEnabled(false)
		.AddSeparator()
		.AddItem("Usuń z listy", CONTACTMENU_REMOVE)
			.SetEnabled(false)
		.AddItem("Ignoruj", CONTACTMENU_IGNORE)
			.SetEnabled(false)
	;

	menu->SetTargetForItems(this);
	// We use the iListView screen context, since that's where the message 
	// originated from
	iListView->ConvertToScreen(&where);
	menu->Go(where, true, true, true);
}
