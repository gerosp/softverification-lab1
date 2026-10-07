// 37. Цикл с безусловным break
// Цель: Условие всегда истинно, но тело выполняется один раз.
// В графе: Структура цикла не означает достижимое обратное ребро: каждый путь тела заканчивается break.
int SinglePass(int step)
{
    int total = 0;
    while (true)
    {
        total = total + step;
        break;
    }
    return total;
}
