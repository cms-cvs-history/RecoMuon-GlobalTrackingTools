import FWCore.ParameterSet.Config as cms

from RecoMuon.TrackingTools.MuonServiceProxy_cff import *
from RecoMuon.GlobalTrackingTools.GlobalMuonRefitter_cff import *

glbTrackQual = cms.EDProducer(
    "GlobalTrackQualityProducer",
    MuonServiceProxy,
    InputCollection = cms.InputTag("globalMuons"),
    BaseLabel = cms.string('GLB'),
    RefitterParameters = cms.PSet(
    GlobalMuonRefitter
    ),
    nSigma = cms.double(3.0),
    MaxChi2 = cms.double(100000.0),
    )
