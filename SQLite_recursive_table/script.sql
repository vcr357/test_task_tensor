WITH RECURSIVE hierarchy(
    id,
    parent_id,
    full_name,
    short_name,
    full_name_lc,
    short_name_lc,
    match_id,
    depth
) AS
(
    SELECT
        id,
        parent_id,
        full_name,
        short_name,
        full_name_lc,
        short_name_lc,
        id AS match_id,
        0 AS depth
    FROM items
    WHERE full_name_lc LIKE 'това%'
       OR short_name_lc LIKE 'това%'
    UNION ALL
    SELECT
        parent.id,
        parent.parent_id,
        parent.full_name,
        parent.short_name,
        parent.full_name_lc,
        parent.short_name_lc,
        hierarchy.match_id,
        hierarchy.depth + 1
    FROM hierarchy
    JOIN items AS parent
        ON parent.id = hierarchy.parent_id
        )
SELECT
    id,
    parent_id,
    full_name,
    short_name,
    match_id,
    depth
FROM hierarchy
ORDER BY full_name_lc ASC, depth DESC;