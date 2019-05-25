# Description: a dummy worker that will "scan" the files given for viruses
# and give the answer randomly (whats the difference hahahhaaaa)

from PyQt5.QtCore import pyqtProperty, pyqtSignal, pyqtSlot, QUrl
from PyQt5.QtQml import QQmlListProperty
from time import sleep
from random import choice

from worker import ProgressWorker
from threatmodel import ThreatModel

class ScanWorker(ProgressWorker):
    """
    Description: A virus scanner.
    Usage: Create an instance in QML, assign the files property, and run.
           The result will be in the result property
    """

    #Q_SIGNALS
    filesChanged = pyqtSignal(QQmlListProperty)
    resultChanged = pyqtSignal(QQmlListProperty)

    def __init__(self, parent=None):
        super().__init__(end=29, start=0, parent=parent)
        self._files = []
        self._result = []


    #property setters
    def setFiles(files):
        self._files = files
        self.filesChanged.emit(self.files)
    def setResult(result):
        self._result = result
        self.resultChanged.emit(self.result)


    #Q_PROPERTY
    @pyqtProperty(QQmlListProperty, fset=setFiles, notify=filesChanged)
    def files(self):
        return QQmlListProperty(QUrl, self, self._files)
    @pyqtProperty(QQmlListProperty, notify=resultChanged)
    def result(self):
        return QQmlListProperty(ThreatModel, self, self._result)

    def run(self):
        for i in range(30):
            sleep(0.5)
            self.log_line("slept for {}".format(i*2 + 2))
            self.advance()
