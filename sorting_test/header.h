template<typename T>
struct Comparator
{
    int operator()(T *e1, T *e2)
    {
        return *e1 - *e2;
    }
};

template<typename T, typename COMP>
void sort(T *items, int count)
{
    if (count <= 1)
        return;

    T *pivot = &items[count-1];
    int i = -1;
    int j = 0;

    while (j < count-1)
    {
        int comp = COMP()(&items[j], pivot);
        if (comp <= 0)
        {
            ++i;
            T tmp = items[i];
            items[i] = items[j];
            items[j] = tmp;
        }
        ++j;
    }

    T tmp = items[i+1];
    items[i+1] = items[count-1];
    items[count-1] = tmp;

    sort<T, COMP>(items, i+1);
    sort<T, COMP>(&items[i+2], count-i-2);
}
