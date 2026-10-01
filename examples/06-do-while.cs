int Count(int limit, bool enabled)
{
    int count = 0;
    bool repeat = enabled;
    do
    {
        count++;
        repeat = enabled && count < limit;
    }
    while (repeat || count < 2);
    return count;
}
