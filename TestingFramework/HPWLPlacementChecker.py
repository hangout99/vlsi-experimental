import Experiment_HPWL
from Experiment_HPWL import *

import Parameters
from Parameters import *

class HPWLPlacementChecker(Experiment_HPWL):
    masterLogFolder = ''

    def __init__(self, name, cfg, benchmarks, masterLogFolder = '', cmdLine = ''):
        Experiment_HPWL.__init__(self, name, cfg, benchmarks, cmdLine)
        self.masterLogFolder = masterLogFolder

    def ParseLogAndFillTable(self, logName, benchmark, reportTable):
        #define metrics
        HPWL     = 0
        workTime = 0
        result   = ''

        #collect metrics
        parser = LogParser(logName)
        HPWL = str(parser.GetFromPFST('DP', 'HPWL'))

        masterLogName = self.masterLogFolder + "/" + os.path.basename(logName)
        masterParser = LogParser(masterLogName)
        masterHPWL = str(masterParser.GetFromPFST('DP', 'HPWL'))
        compare_result = CompareValues(HPWL, masterHPWL)

        if (compare_result == 'notEqual'):
                  result = 'Changed'

        #print metrics
        table = open(reportTable, 'a')
        table.write('\n')
        printStr = benchmark + ';'
        printStr += str(HPWL).replace('.', ',') + result
        print(printStr)
        table.write(printStr)

        table.close()

def test():
    testRunner = TestRunner()

    e = HPWLPlacementChecker('ISPD04 experiment', 'hpwl_ispd04.cfg', 'ISPD04.list', "../master logs/HPWL/ispd")
    #e = HPWLPlacementChecker('IWLS05 HPWL experiment', 'hpwl_iwls05.cfg', 'IWLS05_fast.list', "../master logs/HPWL/iwls")
    testRunner.parameters.experiments.append(e)

    testRunner.Run()

#test()