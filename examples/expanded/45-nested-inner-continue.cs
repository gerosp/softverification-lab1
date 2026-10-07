// 45. Continue внутреннего цикла
// Цель: Пропуск диагональных пар, внешняя итерация продолжается.
// В графе: continue направлен к column++, а не к row++.
int CountWithoutDiagonal(int rows, int columns)
{
    int total = 0;
    for (int row = 0; row < rows; row++)
    {
        for (int column = 0; column < columns; column++)
        {
            if (row == column)
            {
                continue;
            }
            total++;
        }
    }
    return total;
}
