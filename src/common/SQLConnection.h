#ifndef SQL_CONNECTION_H
#define SQL_CONNECTION_H

#include <string>
#include <OS.h>

#define USE_SQLITE
//#define USE_POSTGRES

/**
	Abstract SQL connection class.
	
	Sub-classes implement the actual connection to a specific
	database.
*/
class SQLConnection
{
	public:
		SQLConnection() 
			{};
		virtual ~SQLConnection() 
			{};
		
		/**
			Perform a SQL statement on the server.
			
			@param query The query to be performed
		*/
		virtual status_t Exec(string query)=0;
		
		
		/**
			Get a string describing the result of the last
			call to Exec()
		*/
		virtual string GetLastError()=0;

		/**
			Count the number of rows in the latest reply from
			the server.
		*/
		virtual int NumRows()=0;
		
		/**
			Get a value from a row in the latest reply

			@param row Row offset
			@param field Get value of the Nth field
		*/

		virtual string GetValue(int row, int field)=0;
		/**
			Get a value from a row in the latest reply
			
			@param row Row offset
			@param fieldname Name of the field to get value from
		*/
		virtual string GetValue(int row, string fieldname)=0;
};

#ifdef USE_SQLITE

#ifndef usleep
#define ADD_usleep
extern "C" void usleep( unsigned long t );
#endif

#include "sqlite3.h"

class SQLiteConnection : public SQLConnection
{
	private:
		sqlite3 *	m_sqlite;
		int			m_last_error;
		char **		m_table;
		int			m_rows;
		int			m_cols;
		
		/* 
			Frees all memory that's been allocated by the last query
			as needed. Call at the start of Exec(), in destructor etc.
		*/
		void Cleanup();
		
	public:
		SQLiteConnection( const char * filename );
		virtual ~SQLiteConnection();
		
		/**
			Perform a SQL statement on the server.
			
			@param query The query to be performed
		*/
		virtual status_t Exec(string query);
		
		/**
			Get a string describing the result of the last
			call to Exec()
		*/
		virtual string GetLastError();
		
		/**
			Count the number of rows in the latest reply from
			the server.
		*/
		virtual int NumRows();
		
		/**
			Get a value from a row in the latest reply

			@param row Row offset
			@param field Get value of the Nth field
		*/
		virtual string GetValue(int row, int field);
		
		/**
			Get a value from a row in the latest reply
			
			@param row Row offset
			@param fieldname Name of the field to get value from
		*/
		virtual string GetValue(int row, string fieldname);
		
		/**
			Pretty-print the results of the last query
		*/
		void PrettyPrintResults();
};
#endif // END OF USE_SQLITE


#endif
