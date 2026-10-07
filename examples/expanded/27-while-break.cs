// 27. Break в одном while
// Цель: Один цикл и одна точка досрочного выхода.
// В графе: break ведёт за while, минуя увеличение счётчика и повторную проверку.
int CountWithWhileBreak(int n, int limit)
{
    int count = 0;
    while (count < n)
    {
        if (count >= limit)
        {
            break;
        }
        count++;
    }
    return count;
}
