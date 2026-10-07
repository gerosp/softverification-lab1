// 11. Цепочка else if
// Цель: Три взаимоисключающих пути с общим продолжением.
// В графе: Второй предикат достижим только при ложном первом; три определения result сливаются.
int ClassifySign(int x)
{
    int result;
    if (x < 0)
    {
        result = -1;
    }
    else if (x == 0)
    {
        result = 0;
    }
    else
    {
        result = 1;
    }
    return result;
}
