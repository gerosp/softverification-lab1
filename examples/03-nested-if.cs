int Classify(int x, bool enabled)
{
    if (!enabled)
        return 0;

    int result = 1;
    if (x > 0)
    {
        if (x > 10)
            result = 3;
        else
            result = 2;
    }
    else if (x < 0)
    {
        result = -1;
    }
    return result;
}
