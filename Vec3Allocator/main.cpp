#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <cstdlib>

#include "Vec3Allocator.hpp"
#include <iostream>
#include <cstdint>
#include <utility>
#include <cassert>

int main()
{

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);

    // Проверка размера и выравнивания

    {
        Vec3Allocator allocator;

        Vec3 *p = allocator.Allocate();

        const auto address =
            reinterpret_cast<std::uintptr_t>(p);

        std::cout << "Vec3 address: 0x"
                  << std::hex
                  << address
                  << std::dec
                  << '\n';

        assert(address % 16 == 0);

        std::cout << "Alignment test: OK\n";

        allocator.Deallocate(p);
    }

    //  Проверка многосегментности:
    //  В одном сегменте 5 ячеек,выделяем 11 объектов, необходимо 3 сегмента.

    {
        Vec3Allocator allocator;

        Vec3 *objects[11];

        for (int i = 0; i < 11; ++i)
        {
            objects[i] = allocator.Allocate();

            assert(objects[i] != nullptr);

            auto address =
                reinterpret_cast<std::uintptr_t>(objects[i]);

            assert(address % 16 == 0);
        }

        assert(allocator.SegmentsCount() == 3);

        std::cout << "Multi-segment test: OK\n";

        for (Vec3 *p : objects)
        {
            allocator.Deallocate(p);
        }
    }

    // Проверка переиспользования ячейки
    {
        Vec3Allocator allocator;

        Vec3 *a = allocator.Allocate();
        Vec3 *b = allocator.Allocate();

        allocator.Deallocate(a);

        // Следующий аллок должен вернуть именно a тк деаллок добавляет ячейку в начало freeLst
        Vec3 *c = allocator.Allocate();

        assert(c == a);
        assert(c != b);

        std::cout << "Reuse test: OK\n";

        allocator.Deallocate(b);
        allocator.Deallocate(c);
    }

    // Проверка заполнения/освобождения большого количества объектов.
    {
        Vec3Allocator allocator;

        constexpr int N = 1000;

        Vec3 *objects[N];

        for (int i = 0; i < N; ++i)
        {
            objects[i] = allocator.Allocate();

            objects[i]->x = static_cast<float>(i);
            objects[i]->y = static_cast<float>(i + 1);
            objects[i]->z = static_cast<float>(i + 2);

            auto address =
                reinterpret_cast<std::uintptr_t>(objects[i]);

            assert(address % 16 == 0);
        }

        for (int i = 0; i < N; ++i)
        {
            assert(objects[i]->x == static_cast<float>(i));
            assert(objects[i]->y == static_cast<float>(i + 1));
            assert(objects[i]->z == static_cast<float>(i + 2));
        }

        for (int i = 0; i < N; ++i)
        {
            allocator.Deallocate(objects[i]);
        }

        // Освободили ячейки, они вернулись в сегменты (сегментов в POOLSIZE меньше, чем ячеек)
        assert(allocator.SegmentsCount() == 200);
        std::cout << "Stress test: OK\n";
    }

    // reuse test
    {
        Vec3Allocator allocator;

        Vec3 *a = allocator.Allocate();
        Vec3 *b = allocator.Allocate();
        Vec3 *c = allocator.Allocate();
        Vec3 *d = allocator.Allocate();
        Vec3 *e = allocator.Allocate();

        assert(allocator.SegmentsCount() == 1);

        allocator.Deallocate(a);
        allocator.Deallocate(b);
        allocator.Deallocate(c);
        allocator.Deallocate(d);
        allocator.Deallocate(e);

        // Повторно выделяем 5 объектов - новый Segment создаваться не должен.
        Vec3 *a2 = allocator.Allocate();
        Vec3 *b2 = allocator.Allocate();
        Vec3 *c2 = allocator.Allocate();
        Vec3 *d2 = allocator.Allocate();
        Vec3 *e2 = allocator.Allocate();

        assert(allocator.SegmentsCount() == 1);

        allocator.Deallocate(a2);
        allocator.Deallocate(b2);
        allocator.Deallocate(c2);
        allocator.Deallocate(d2);
        allocator.Deallocate(e2);

        std::cout << "Pool reuse without new segment: OK\n";
    }

    // Проверка move-присваивания (operator=(&&))
    {
        Vec3Allocator a;
        Vec3Allocator b;

        Vec3 *p = a.Allocate();
        Vec3 *old_b_obj = b.Allocate();

        assert(a.SegmentsCount() == 1);
        assert(b.SegmentsCount() == 1);

        // У b уже был свой сегмент — он должен быть корректно освобождён
        // перед тем как b примет ресурсы a (иначе — утечка).
        b.Deallocate(old_b_obj);

        b = std::move(a);

        // a опустошён, ничем не владеет
        assert(a.SegmentsCount() == 0);

        // b теперь владеет тем, что раньше было у a
        assert(b.SegmentsCount() == 1);

        Vec3 *q = b.Allocate();
        assert(q != nullptr);

        // p был выделен из старого сегмента a, который теперь принадлежит b —
        // деаллокация должна пройти корректно.
        b.Deallocate(p);
        b.Deallocate(q);

        std::cout << "Move-assignment test: OK\n";
    }

    // Проверка self-move-присваивания
    {
        Vec3Allocator c;
        Vec3 *r = c.Allocate();
        assert(c.SegmentsCount() == 1);
        c = std::move(c);

        // Благодаря проверке (this != &other) состояние не должно быть разрушено.
        assert(c.SegmentsCount() == 1);

        c.Deallocate(r);

        std::cout << "Self-move-assignment test: OK\n";
    }

    // В конце все аллокаторы уничтожены живых сегментов должно быть 0

    std::cout << "No live segments: OK\n";
    std::cout << "All tests passed.\n";

// проверка на доступ к памяти через директиву
//"/fsanitize=address"
// на утечку - CRT Debug Heap

// специально утечка, чтобы проверить что тест утечки работает
#define FORCED_LEAK ///<- закомментировать для отключения принудительной утечки!
#ifdef FORCED_LEAK
    {
        Vec3Allocator *leaked = new Vec3Allocator();
        leaked->Allocate();
        std::cout << "\n\n------------->Hand-made memory leak here!!!! IS NOT AN ERROR!!!!!!!!!!!\n";
        std::cout << "------------->Disable FORCED_LEAK in main.cpp to switch off leak test \n\n";
    }
#endif

    return 0;
}