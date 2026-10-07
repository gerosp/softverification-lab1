// 42. Два вложенных while
// Цель: Вложенные while с явным обновлением каждого счётчика.
// В графе: Выход внутреннего цикла ведёт к row++, а его обратное ребро — к условию column.
int CountRectangleWhile(int rows, int columns)
{
    int total = 0;
    int row = 0;
    while (row < rows)
    {
        int column = 0;
        while (column < columns)
        {
            total++;
            column++;
        }
        row++;
    }
    return total;
}
