// 47. Зависимая граница внутреннего цикла
// Цель: Число внутренних итераций зависит от внешнего счётчика.
// В графе: Предикат column <= row зависит от определения row внешнего цикла.
int CountTriangle(int n)
{
    int total = 0;
    for (int row = 0; row < n; row++)
    {
        for (int column = 0; column <= row; column++)
        {
            total++;
        }
    }
    return total;
}
