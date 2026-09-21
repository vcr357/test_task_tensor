
#include <cstddef> // std::byte
#include <utility> // std::exchange

#define _TESTING_

typedef struct
{
    float x;
    float y;
    float z;

} Vec3;

class Vec3Allocator
{
private:
    static constexpr int POOLSIZE = 5;
    int live_segments = 0;


    // Свободные ячейки пула
    struct FreeNode
    {
        FreeNode *next;
    };

    // Начало списка свободных ячеек.
    FreeNode *freeList = nullptr;

    // Ячейка. alignas(16) гарантирует &cell % 16 ==0
    struct alignas(16) Cell
    {
        union
        {
            Vec3 value;
            FreeNode free;
        };
    };

    // Пул памяти на несколько ячеек
    struct Segment
    {
        Segment *next;
        Cell cells[POOLSIZE];
    };

    // Собссно список всех сегментов.
    Segment *segmentsList = nullptr;

    // Когда кончились свободные ячейки или когда еще не создано ни одного сегмента
    void NewSegment()
    {
        // выделяем память в сегменте POOLSIZE ячеек по 16 байт + служебное поле next.
        Segment *segment = new Segment;
        
        ++live_segments;        

        // Добавляем сегмент в начало списка
        segment->next = segmentsList;
        segmentsList = segment;

        // Все ячеqки нового сегмента изначально свободны
        // Строим из них цепочку
        for (int i = 0; i < POOLSIZE - 1; i++)
        {
            segment->cells[i].free.next = &segment->cells[i + 1].free;
        }

        segment->cells[POOLSIZE - 1].free.next = freeList;

        // Теперь первая ячейка нового сегмента начало общего free-list.
        freeList = &segment->cells[0].free;
    }

    void FreeSegments() noexcept
    {
        Segment *segment = segmentsList;

        while (segment != nullptr)
        {
            Segment *next = segment->next;

            delete segment;

            --live_segments;

            segment = next;
        }

        segmentsList = nullptr;
        freeList = nullptr;
    }

public:
    Vec3Allocator() = default;

    // Копировать аллокатор нельзя:
    // иначе два аллокатора будут владеть одними и теми же сегментами
    // и при удалении копии неизбежно получим утечку
    // Глубокое копирование тоже выглядит странно
    // поскольку непонятно что делать с уже выделенными векторами
    // Ну и по условию STL allocator API не требуется, так что с allocate_shared можно не возиться
    Vec3Allocator(const Vec3Allocator &) = delete;
    Vec3Allocator &operator=(const Vec3Allocator &) = delete;

    // Move-assigment передаёт указатели segments_ и free_, исходный аллокатор опусташаем.
    Vec3Allocator &operator=(Vec3Allocator &&other) noexcept
    {
        if (this != &other)
        {
            FreeSegments();

            // Забираем ресурсы other.
            segmentsList = std::exchange(other.segmentsList, nullptr);
            freeList = std::exchange(other.freeList, nullptr);
            live_segments = std::exchange(other.live_segments, 0);
        }

        return *this;
    }

    // Move-constructor
    Vec3Allocator(Vec3Allocator &&other) noexcept
        : segmentsList(std::exchange(other.segmentsList, nullptr))
          ,freeList(std::exchange(other.freeList, nullptr))
          ,live_segments(std::exchange(other.live_segments, 0))

    {
    }

    Vec3 *Allocate()
    {
        // Если свободных ячеек нет — добавляем новый сегмент.
        if (freeList == nullptr)
        {
            NewSegment();
        }

        // Берём первую свободную ячейку.
        FreeNode *node = freeList;

        // Переставляем голову freelist на следующую ячейку.
        freeList = node->next;

        // Теперь эта ячейка считается занятой.
        return reinterpret_cast<Vec3 *>(node);
    }

    void Deallocate(Vec3 *ptr) noexcept
    {
        // Обработка ошибочных/чужих указателей в deallocate() не требуется.
        //  if (ptr == nullptr)
        //      return;

        // Возвращаем ячейку обратно в начало freelist.
        auto *node = reinterpret_cast<FreeNode *>(ptr);

        node->next = freeList;
        freeList = node;
    }

    int SegmentsCount() const noexcept
    {
        return live_segments;        
    }

    ~Vec3Allocator()
    {
        FreeSegments();
    }
};