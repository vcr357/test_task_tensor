cd bin
sqlite3.exe "..\db\tensor_test.db" < "..\create_db.sql"
sqlite3.exe "..\db\tensor_test.db" "PRAGMA table_info(items);"
pause