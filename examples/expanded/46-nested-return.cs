// 46. Return из вложенного цикла
// Цель: Первое совпадение завершает метод из двух вложенных циклов.
// В графе: return обходит оба шага и все последующие проверки, а не выходит только из внутреннего цикла.
int FindPairIndex(int rows, int columns, int target)
{
    for (int row = 0; row < rows; row++)
    {
        for (int column = 0; column < columns; column++)
        {
            if (row + column == target)
            {
                return row * columns + column;
            }
        }
    }
    return -1;
}
