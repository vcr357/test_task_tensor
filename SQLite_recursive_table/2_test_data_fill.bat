cd bin
sqlite3.exe "..\db\tensor_test.db" < "..\test_data.sql"
sqlite3.exe "..\db\tensor_test.db" "SELECT id, parent_id, full_name, short_name, full_name_lc, short_name_lc FROM items ORDER BY id;"
pause