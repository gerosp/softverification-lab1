// 39. Цикл внутри if
// Цель: Весь цикл находится в одной условной ветви.
// В графе: При enabled = false обходится и тело, и условие while.
int ConditionalLoop(int n, bool enabled)
{
    int count = 0;
    if (enabled)
    {
        while (count < n)
        {
            count++;
        }
    }
    return count;
}
