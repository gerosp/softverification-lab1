// 41. Два вложенных for
// Цель: Первое появление вложенных циклов, без ранних переходов.
// В графе: Внутренний счётчик инициализируется заново на каждой внешней итерации.
int CountRectangleFor(int rows, int columns)
{
    int total = 0;
    for (int row = 0; row < rows; row++)
    {
        for (int column = 0; column < columns; column++)
        {
            total++;
        }
    }
    return total;
}
