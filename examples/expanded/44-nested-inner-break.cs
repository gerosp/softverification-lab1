// 44. Break внутреннего цикла
// Цель: Ранее изученный break перенесён во вложенный цикл.
// В графе: break выходит только из внутреннего for и ведёт к шагу внешнего for.
int CountWithInnerBreak(int rows, int columns, int limit)
{
    int total = 0;
    for (int row = 0; row < rows; row++)
    {
        for (int column = 0; column < columns; column++)
        {
            if (column >= limit)
            {
                break;
            }
            total++;
        }
    }
    return total;
}
