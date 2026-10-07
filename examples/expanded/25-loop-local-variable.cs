// 25. Локальная переменная в теле
// Цель: Временная переменная объявляется внутри тела цикла.
// В графе: doubled определяется перед каждым чтением в теле, не переносится между итерациями.
int SumLocalValues(int n)
{
    int total = 0;
    for (int i = 0; i < n; i++)
    {
        int doubled = i * 2;
        total = total + doubled;
    }
    return total;
}
