// 34. Две точки break
// Цель: Разные предикаты выходят из одного и того же цикла.
// В графе: Оба break имеют одну цель; второй if недостижим при истинном первом.
int SumWithTwoBreaks(int n, int firstStop, int secondStop)
{
    int total = 0;
    for (int i = 0; i < n; i++)
    {
        if (i == firstStop)
        {
            break;
        }
        if (i == secondStop)
        {
            break;
        }
        total = total + i;
    }
    return total;
}
