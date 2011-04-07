import CoreScripts
from CoreScripts import *

import Parameters
from Parameters import *

class Experiment_Joint(BaseExperiment):
    def __init__(self):
        metrics = ['Time', 'Cells', 'HPWL', 'TWL', 'TNS', 'WNS']
        stages  = ['INIT', 'GP', 'NBUF', 'LEGB']

        BaseExperiment.__init__(self, 'Joint Algorithm', 'JointPlacementBuffering.cfg', 'IWLS_GP_r1511_joint_fast.list', metrics, stages)

def TestRun():
    plotterFlag = "--plotter.enabled=false"

    experimentWithoutBuffering = Experiment_Joint()
    experimentWithoutBuffering.name = "GP + Buffering"
    experimentWithoutBuffering.cmdArgs = ["--LSE.GlobalPlacement.UseBuffering=false", "--New_Buffering.TypeNetListBuffering=0", plotterFlag]
    
    #experimentWithBuffering = Experiment_Joint()
    #experimentWithBuffering.name = "Joint GP + Buffering"
    #experimentWithBuffering.cmdArgs = ["--LSE.GlobalPlacement.UseBuffering=true", plotterFlag]
    
    experimentWithBuffering = Experiment_Joint()
    experimentWithBuffering.name = "Buffering Path-Based"
    experimentWithBuffering.cmdArgs = ["--LSE.GlobalPlacement.UseBuffering=false", plotterFlag, "--New_Buffering.TypeNetListBuffering=1"]

    testRunner = TestRunner()
    testRunner.AddExperimentToGroup(experimentWithoutBuffering)
    testRunner.AddExperimentToGroup(experimentWithBuffering)
    testRunner.Run()

if (__name__ == "__main__"):
    TestRun()