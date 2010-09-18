import subprocess
import sys
import os

import datetime
from datetime import date
import time

from Emailer import *
from SvnWorker import *
from ReportCreator import *

import CoreFunctions
from CoreFunctions import *

import Parameters
from Parameters import *

import BaseExperiment
from BaseExperiment import *

TERMINATED = 'Terminated'

class TestRunner:
    parameters = ''
    comment    = ''
    experimentResult = OK

    experimentsToCompare = {} #Group of experiments. Theit results will be compared

    newBenchmarks        = ''
    failedBenchmarks     = ''
    terminatedBenchmarks = ''

    nOkBenchmarks         = 0
    nNewBenchmarks        = 0
    nFailedenchmarks      = 0
    nChangedBenchmarks    = 0
    nTerminatedBenchmarks = 0

    def __init__(self, parameters = TestRunnerParameters()):
        self.parameters = parameters
        self.parameters.experiments = []

    def BuildSln(self, slnPath, mode = "Rebuild"):
        print('Building solution...')
        args = [Tools.MSBuild, slnPath, '/t:' + mode, '/p:Configuration=Release']
        subprocess.Popen(subprocess.list2cmdline(args)).communicate()

    def ExtractBenchmarkList(self, benchmarksListPath):
        benchmarks = (open(benchmarksListPath).read()).split('\n')

        # Perform filtering of empty lines and commented by # benchmarks
        benchmarks = [x for x in benchmarks if not x.strip().startswith('#')]
        benchmarks = [x for x in benchmarks if len(x.strip())]

        return benchmarks

    def PrintXXXBenchmarks(self, status, nXXXBenchmarks, benchmarks = ''):
        if (nXXXBenchmarks == 0):
            return ('')

        printStr = status + str(nXXXBenchmarks)

        if (benchmarks != ''):
            printStr += '(' + benchmarks + ')'

        return (printStr + '\n')

    def Append(self, newExperiment):
         self.parameters.experiments.append(newExperiment)

    def AddExperimentToGroup(self, newExperiment):
        self.Append(newExperiment)

        if (self.experimentsToCompare != {}):
            groupExp = list(self.experimentsToCompare.keys())[0]

            if (newExperiment.benchmarks != groupExp.benchmarks):
                print("list files are not equal")
                exit(1)

            if (newExperiment.metrics != groupExp.metrics):
                print("metrics are not equal")
                exit(1)

            if (newExperiment.stages != groupExp.stages):
                print("stages are not equal")
                exit(1)

        self.experimentsToCompare[newExperiment] = {}

    def CompareExperimentsInGroup(self, resultFileName):
        groupExp = list(self.experimentsToCompare.keys())[0]
        stages   = groupExp.stages
        metrics  = groupExp.metrics

        finalStageIdx = len(metrics) - 1

        #Create header of the table
        #---First string of the header------------------
        cols = []
        cols.append(END_OF_COLUMN)
        cols.append('INIT')

        for col in range(len(metrics)):
            cols.append(END_OF_COLUMN)

        for experiment in self.experimentsToCompare.keys():
            cols.append(END_OF_COLUMN)
            cols.append(experiment.name)

            for col in range(len(metrics)):
                cols.append(END_OF_COLUMN)

        WriteStringToFile(cols, resultFileName)

        #---Second string of the header------------------
        cols = ['benchmark']
        cols.append(END_OF_COLUMN)

        for col in range(len(self.experimentsToCompare.keys()) + 1):
            for row in range(len(metrics)):
                cols.append(metrics[row])
                cols.append(END_OF_COLUMN)

            cols.append(END_OF_COLUMN)

        WriteStringToFile(cols, resultFileName)

        #------Print results-------------------------------------
        for benchmark in self.experimentsToCompare[groupExp].keys():
            cols = [benchmark]
            cols.append(END_OF_COLUMN)
            #TODO: mark experiment with best result for each metric

            for experiment in self.experimentsToCompare.keys():
                resultValues = self.experimentsToCompare[experiment][benchmark]

                if (len(cols) == 2):
                    for col in range(len(metrics)):
                        cols.append(str(resultValues[0][col]))
                        cols.append(END_OF_COLUMN)

                    cols.append(END_OF_COLUMN)

                #TODO: else check intitial values

                #for row in range(1, len(stages)):
                for col in range(len(metrics)):
                    cols.append(str(resultValues[finalStageIdx][col]))
                    cols.append(END_OF_COLUMN)

                cols.append(END_OF_COLUMN)

            cols.append(END_OF_COLUMN)
            WriteStringToFile(cols, resultFileName)

    def RunExperiment(self, experiment):
        benchmarks = self.ExtractBenchmarkList(experiment.benchmarks)

        print('Config: %s' % experiment.cfg)
        print('List:   %s' % experiment.benchmarks)
        print("Benchmarks:\n" + ", ".join(benchmarks))
        print("\n")

        reportCreator = ReportCreator(experiment.cfg)
        logFolder     = reportCreator.CreateLogFolder()
        reportTable   = reportCreator.GetReportTableName()

        experiment.CreateEmptyTable(reportTable)
        self.experimentResult = OK

        for benchmark in benchmarks:
            logFileName = logFolder + "/" + os.path.basename(benchmark) + ".log"
            fPlacerOutput = open(logFileName, 'w');
            resultValues = []

            defFile = "--params.def=" + os.path.dirname(os.path.abspath(experiment.benchmarks)) + "/" + benchmark + ".def"
            params = [GeneralParameters.binDir + "itlPlaceRelease.exe", os.path.abspath(experiment.cfg), defFile, experiment.cmdLine]
            #HACK: ugly hack for ISPD04 benchmarks
            if experiment.cfg.find("ispd04") != -1:
                lefFile = "--params.lef=" + os.path.dirname(os.path.abspath(experiment.benchmarks)) + "/" + benchmark + ".lef"
                params.append(lefFile)

            benchmarkResult = ''
            #subprocess.call(params, stdout = fPlacerOutput, cwd = GeneralParameters.binDir)
            p = subprocess.Popen(params, stdout = fPlacerOutput, cwd = GeneralParameters.binDir)
            t_start = time.time()
            seconds_passed = 0

            while(not p.poll() and seconds_passed < self.parameters.maxTimeForBenchmark):
                seconds_passed = time.time() - t_start

            retcode = p.poll()

            if (retcode == None):
                print("Time out on " + benchmark)
                benchmarkResult            = TERMINATED
                self.terminatedBenchmarks  += ' ' + benchmark + ';'
                self.nTerminatedBenchmarks += 1
                self.experimentResult      = TERMINATED
                p.terminate()

                if (self.nTerminatedBenchmarks >= 3):
                    self.comment += 'Reached maximum number of terminated benchmarks\n'
                    return (reportTable)

            else:
                (benchmarkResult, resultValues) = experiment.ParseLogAndFillTable(logFileName, benchmark, reportTable)

            fPlacerOutput.close()
            print(benchmark + ' DONE')

            if (experiment in self.experimentsToCompare):
                self.experimentsToCompare[experiment][benchmark] = resultValues

            if (benchmarkResult == FAILED):
                self.experimentResult  = FAILED
                self.failedBenchmarks  += ' ' + benchmark + ';'
                self.nFailedBenchmarks += 1

            elif (benchmarkResult == CHANGED):
                self.nChangedBenchmarks += 1

                if ((self.experimentResult != FAILED) and (self.experimentResult != TERMINATED)):
                    self.experimentResult = CHANGED

            elif (benchmarkResult == OK):
                self.nOkBenchmarks += 1

            elif (benchmarkResult == NEW):
                self.newBenchmarks  += ' ' + benchmark + ';'
                self.nNewBenchmarks += 1

            #else TERMINATED - do nothing

        return reportTable

    def Run(self):
        cp      = CoolPrinter()
        svn     = SvnWorker()
        emailer = Emailer()

        subject = 'Night experiments'
        text    = ''
        attachmentFiles = []

        cp.CoolPrint('Start')

        if self.parameters.doCheckout:
            cp.CoolPrint('Delete sources and Checkout')
            svn.DeleteSources(GeneralParameters.checkoutPath)
            retcode = 1

            for i in range(10):
                #TODO: implement non HEAD revision
                retcode = svn.CheckOut(RepoParameters.srcRepoPath, GeneralParameters.checkoutPath)

                if retcode == 0:
                    break

            if retcode != 0:
                text = 'svn error: checkout failed!'

                if (self.parameters.doSendMail == True):
                    emailer.PrepareAndSendMail(subject, text, attachmentFiles)

                return -1

        if self.parameters.doBuild:
            cp.CoolPrint('Build')
            self.BuildSln(GeneralParameters.slnPath)

        for experiment in self.parameters.experiments:
            startTime = GetTimeStamp()
            print('Start time: ' + startTime)
            self.experimentResult = OK
            self.comment = ''

            self.newBenchmarks        = ''
            self.failedBenchmarks     = ''
            self.terminatedBenchmarks = ''

            self.nOkBenchmarks         = 0
            self.nNewBenchmarks        = 0
            self.nChangedBenchmarks    = 0
            self.nFailedBenchmarks     = 0
            self.nTerminatedBenchmarks = 0

            cp.CoolPrint(experiment.name)
            reportTable = self.RunExperiment(experiment)
            #cp.CoolPrint("Sending mail with " + reportTable)

            #subject += ' ' + experiment.name
            #text += experiment.name + ': ' + self.experimentResult
            text += experiment.name + ', ' + os.path.basename(experiment.cfg)
            text += ', ' + os.path.basename(experiment.benchmarks) + ' ('
            nBenchmarks = self.nOkBenchmarks + self.nChangedBenchmarks + self.nFailedBenchmarks + self.nTerminatedBenchmarks + self.nNewBenchmarks
            text += str(nBenchmarks) + ' benchmark(s)):\n'
            text += 'Start time: ' + startTime + '\n'
            text += self.PrintXXXBenchmarks('Ok:         ', self.nOkBenchmarks)
            text += self.PrintXXXBenchmarks('Changed:    ', self.nChangedBenchmarks)
            text += self.PrintXXXBenchmarks('Failed:     ', self.nFailedBenchmarks, self.failedBenchmarks)
            text += self.PrintXXXBenchmarks('Terminated: ', self.nTerminatedBenchmarks, self.terminatedBenchmarks)
            text += self.PrintXXXBenchmarks('NEW:        ', self.nNewBenchmarks, self.newBenchmarks)

            text += self.comment
            text += '\n'

            if (self.experimentResult == CHANGED):
                attachmentFiles.append(reportTable)
            #emailer.SendResults(experiment, reportTable, self.experimentResult)

        text += 'Finished at ' + GetTimeStamp()
        print(text)
        if (self.parameters.doSendMail == True):
            emailer.PrepareAndSendMail(subject, text, attachmentFiles)

        if (len(self.experimentsToCompare) > 1):
            cp.CoolPrint('Comaparing experiments')
            reportCreator = ReportCreator('Comaparing')
            logFolder     = reportCreator.CreateLogFolder()
            cmpFileName   = reportCreator.GetReportTableName()
            self.CompareExperimentsInGroup(cmpFileName)

        cp.CoolPrint('Finish')
