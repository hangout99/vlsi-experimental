import os
from ParametersParsing import GeneralParameters
from LogParser import LogParser, PFST, PQAT
from Logger import Logger
from CoreFunctions import END_OF_COLUMN, CreateConfigParser, WriteStringToFile,\
                          MakeTableInPercents, PrintTableToFile

OK      = "Ok"
NEW     = "New"
FAILED  = "Failed"
CHANGED = "Changed"

class ExperimentResults:
  errors           = []
  resultFile       = "" #file with result table
  pfstTables       = {} #benchmark: pfst
  benchmarkResults = {} #result: [benchmarks]

  def __init__(self):
    self.errors           = []
    self.resultFile       = ""
    self.pfstTables       = {}
    self.benchmarkResults = {}

  def GetPFSTForBenchmark(self, benchmark):
    return self.pfstTables[benchmark]

  def AddError(self, error):
    self.errors.append(error)

  def AddBenchmarkResult(self, benchmark, result):
    if (not result in self.benchmarkResults.keys()):
      self.benchmarkResults[result] = []

    self.benchmarkResults[result].append(benchmark)

  def AddPFSTForBenchmark(self, benchmark, table):
    self.pfstTables[benchmark] = table

  def __str__(self):
    resultStr = ""

    for (result, benchmarks) in list(self.benchmarkResults.iteritems()):
      resultStr += ("%s: %s benchmarks (" % (result, len(benchmarks)))

      for benchmark in (benchmarks):
        resultStr += ("%s; " % (benchmark))

      resultStr += ")\n"

    for error in self.errors:
      resultStr += ("%s\n" % (error))

    return (resultStr + "\n")

  def Print(self):
    print(self.__str__())

class BaseExperiment:
  name        = ""
  cfg         = ""
  benchmarks  = ""
  cmdArgs     = [] #list of command line arguments
  metrics     = []
  stages      = []
  doParsePQAT = False

  cfgParser         = CreateConfigParser()
  generalParameters = GeneralParameters(cfgParser)

  def __init__(self, name, cfg, benchmarks, metrics, stages, cmdArgs = []):
    self.name = name
    self.cfg = os.path.join(self.generalParameters.binDir, "cfg", cfg)
    self.benchmarks = self.generalParameters.benchmarkCheckoutPath + benchmarks
    self.cmdArgs = cmdArgs
    self.metrics = metrics
    self.stages  = stages

  def CopyingConstructor(self, be):
    self.cfg         = be.cfg
    self.name        = be.name
    self.stages      = be.stages
    self.metrics     = be.metrics
    self.cmdArgs     = be.cmdArgs
    self.benchmarks  = be.benchmarks
    self.doParsePQAT = be.doParsePQAT

  def SetConfig(self, cfg):
    self.cfg = os.path.join(self.generalParameters.binDir, "cfg", cfg)

  def SetBenchmarksList(self, benchmarks):
    self.benchmarks = os.path.join(self.generalParameters.benchmarkCheckoutPath, benchmarks)

  def CreateEmptyTable(self, reportTable):
    cols = ["Benchmark", END_OF_COLUMN]

    #write header of a table.
    for row in range(len(self.stages)):
      for col in range(len(self.metrics)):
        cols.append("%s_%s" % (self.metrics[col], self.stages[row]))
        cols.append(END_OF_COLUMN)

      cols.append(END_OF_COLUMN) #an empty column between metrics on different stages

    WriteStringToFile(cols, reportTable)

  def ParseLog(self, logName):
    parser = LogParser(logName, PFST, self.cfgParser)
    return (parser.ParsePFST(self.metrics, self.stages))

  def ParsePQATAndPrintTable(self, logName):
    metrics      = ["HPWL", "TNS", "WNS"]
    parser       = LogParser(logName, PQAT, self.cfgParser)
    table        = parser.ParsePQAT(metrics)
    table        = MakeTableInPercents(table)
    pqatName     = r"%s.csv" % (os.path.basename(logName))
    PQATFileName = os.path.join(os.path.dirname(logName), pqatName)
    PrintTableToFile(PQATFileName, table, metrics)

  def AddStringToTable(self, values, benchmark, reportTable):
    cols = [benchmark, END_OF_COLUMN]

    for row in range(len(self.stages)):
      for col in range(len(self.metrics)):
        cols.extend([str(values[row][col]), END_OF_COLUMN])

      cols.append(END_OF_COLUMN) #an empty column between metrics on different stages

    #write metrics to the file
    WriteStringToFile(cols, reportTable)

  def MakeResultTable(self, logFolder, reportTable):
    if (os.path.exists(logFolder) == False):
      logger = Logger()
      logger.Log("Error: folder %s does not exist" % (logFolder))
      return

    reportTable = os.path.join(logFolder, reportTable)
    self.CreateEmptyTable(reportTable)

    for log in os.listdir(logFolder):
      if (os.path.isfile(os.path.join(logFolder, log)) and (".log" == os.path.splitext(log)[-1])):
        benchmark = os.path.splitext(log)[0]
        self.ParseLogAndFillTable(os.path.join(logFolder, log), benchmark, reportTable)

  def ParseLogAndFillTable(self, logName, benchmark, reportTable):
    values = self.ParseLog(logName)

    if (values == []):
      return [FAILED, []]

    self.AddStringToTable(values, benchmark, reportTable)

    if (self.doParsePQAT == True):
      self.ParsePQATAndPrintTable(logName)

    return [OK, values]

def TestResultTableMaking():
  stages     = ["LEG", "DP"]
  metrics    = ["HPWL", "TNS", "WNS"]
  experiment = BaseExperiment("HippocrateDP experiment", "HippocrateDP.cfg", "IWLS_GP_Hippocrate.list",\
                              metrics, stages)

  experiment.MakeResultTable(r"..\Reports\HippocrateDP", "TestTable2.csv")

def test():
  from TestRunner import TestRunner

  stages     = ["LEG", "DP"]
  metrics    = ["HPWL", "TNS", "WNS"]
  experiment = BaseExperiment("HippocrateDP experiment", "HippocrateDP.cfg", "IWLS_GP_Hippocrate.list",\
                              metrics, stages)

  testRunner = TestRunner()
  testRunner.Append(experiment)
  testRunner.Run()

if (__name__ == "__main__"):
  test()