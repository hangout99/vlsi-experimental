def Absolutize(x):
    divisor = x[0]
    if divisor != 0.0:
        for i in range(0, len(x)):
            x[i] = x[i] / divisor
    else:
        for i in range(0, len(x)):
            x[i] = 0.0
    return x

def PrintAbsValues(po, sequence):
    if len(sequence):
        po.write((str(Absolutize(sequence)).replace(", ", ";")[1:-1] +
                ';' + str(min(sequence)) + ';' + str(Average(sequence))).replace(".", ","))
        po.write(2*';')

def Average(values):
    """Computes the arithmetic mean of a list of numbers.
    >>> print(average([20, 30, 70]))
    40.0
    """
    return sum(values) / len(values)

def RemoveDir(dirName):
        RemovePermissions(dirName)
        for name in os.listdir(dirName):
            file = os.path.join(dirName, name)
            if not os.path.islink(file) and os.path.isdir(file):
                RemoveDir(file)
            else:
                RemovePermissions(file)
#                print('remove file')
                os.remove(file)
        try:
            os.rmdir(dirName)
        except WindowsError:
            pass

def RemovePermissions(filePath) :
        if (not os.access(filePath, os.W_OK)):
            os.chmod(filePath, 666)