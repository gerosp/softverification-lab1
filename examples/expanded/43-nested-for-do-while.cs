// 43. For с внутренним do/while
// Цель: Сочетание циклов с проверкой до и после тела.
// В графе: При columns = 0 внутренний цикл делает один проход на каждую внешнюю итерацию.
int CountMixedLoops(int rows, int columns)
{
    int total = 0;
    for (int row = 0; row < rows; row++)
    {
        int column = 0;
        do
        {
            total++;
            column++;
        }
        while (column < columns);
    }
    return total;
}
