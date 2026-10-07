// 50. Комплексный поиск с пропусками
// Цель: Совместная проверка уже знакомых ветвей, вложенных циклов и трёх видов переходов.
// В графе: break завершает внешний цикл, continue ведёт к шагу внутреннего, return завершает метод.
int SearchWithSkips(int rows, int columns, int target)
{
    if (rows <= 0 || columns <= 0)
    {
        return -1;
    }
    int visited = 0;
    for (int row = 0; row < rows; row++)
    {
        if (row > target)
        {
            break;
        }
        for (int column = 0; column < columns; column++)
        {
            if (row == column)
            {
                continue;
            }
            visited++;
            if (row + column == target)
            {
                return visited;
            }
        }
    }
    return -1;
}
