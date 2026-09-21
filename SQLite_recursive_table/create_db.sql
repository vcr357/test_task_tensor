PRAGMA foreign_keys = ON;

CREATE TABLE items (
    id         INTEGER PRIMARY KEY,
    parent_id  INTEGER REFERENCES items(id),
    full_name  TEXT NOT NULL,
    short_name TEXT NOT NULL,
	
-- Ниже колонки для ТЗ заполняем ручками - оказывается SQLite (в винде?) с русскими буковами регистр не меняют.
-- В рабочем решении должен быть управляющий код - слой работы с бд - в котором уже использовать корректный tolower() итп

    full_name_lc TEXT NOT NULL,
    short_name_lc TEXT NOT NULL
);

-- двойной индекс тут прироста не даст, и вроде в SQLite есть OR-оптимизация
CREATE INDEX idx_items_full_name_lc
    ON items (full_name_lc COLLATE BINARY);

CREATE INDEX idx_items_short_name_lc
    ON items (short_name_lc COLLATE BINARY);

CREATE INDEX idx_items_parent_id
    ON items (parent_id);