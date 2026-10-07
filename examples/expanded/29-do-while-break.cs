// 29. Break в одном do/while
// Цель: Break может сработать ещё до первой проверки do/while.
// В графе: break ведёт за цикл и обходит нижний предикат; при limit = 0 увеличение недостижимо.
int CountWithDoBreak(int n, int limit)
{
    int count = 0;
    do
    {
        if (count >= limit)
        {
            break;
        }
        count++;
    }
    while (count < n);
    return count;
}
