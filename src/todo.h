* Add a new component: Sweeper
	This app should run through the database and for each file
	make sure that it still exists and that the index is up to
	date. Pretty much like that Indexer, but using the database
	as the origin instead of the filesystem.
	Probably doesn't have to be a new component, just add an
	option to Indexer, something like -reindex.

* In Indexer:
	Add a call to ANALYZE every two-three hundred indexed files
	to speed things up.

extern "C" _IMPEXP_ROOT int _kset_mon_limit_(int num);

* SQLite

SELECT 
	fileTable.path,contentTable.weight 
FROM 
	fileTable,contentTable,wordTable 
WHERE 
	wordTable.word='eiman' AND 
	contentTable.word=wordTable.id AND 
	contentTable.file=fileTable.id
