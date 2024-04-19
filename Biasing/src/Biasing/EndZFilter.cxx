
#include "Biasing/EndZFilter.h"

/*~~~~~~~~~~~~*/
/*   Geant4   */
/*~~~~~~~~~~~~*/
#include "G4EventManager.hh"
#include "G4RunManager.hh"
#include "G4VTrajectory.hh"
#include "G4VTrajectoryPoint.hh"

/*~~~~~~~~~~~~~*/
/*   SimCore   */
/*~~~~~~~~~~~~~*/
#include "SimCore/UserEventInformation.h"
#include "SimCore/UserTrackInformation.h"

int events = 0;
namespace biasing {

EndZFilter::EndZFilter(const std::string& name,
                                   framework::config::Parameters& parameters)
    : simcore::UserAction(name, parameters) {
      endZ_ = parameters.getParameter<double>("end_z");
}


EndZFilter::~EndZFilter() {}

/*G4ClassificationOfNewTrack EndZFilter::ClassifyNewTrack(
    const G4Track* track, const G4ClassificationOfNewTrack& currentTrackClass) {
  // get the PDGID of the track.
  G4int pdgID = track->GetParticleDefinition()->GetPDGEncoding();

  // Get the particle type.
  G4String particleName = track->GetParticleDefinition()->GetParticleName();

  // Use current classification by default so values from other plugins are not
  // overridden.
  G4ClassificationOfNewTrack classification = currentTrackClass;

  if (track->GetTrackID() == 1 && pdgID == 11) {
    return fWaiting;
  }

  return classification;
}
*/
/*void EndZFilter::stepping(const G4VTrajectory* trajectory) {
  // Get the track associated with this step.
  //auto track{step->GetTrack()};
  //track->GetStep(), every step, find max z-position, use that
  //std::cout << "getPos:" << track->GetVertexPosition().z() << std::endl;
  //events++;
  //std::cout << endZ_ << std::endl;
  //double zpos = track->GetStep()->GetPostStepPoint()->GetPosition().z() ;//- track->GetStep()->GetPreStepPoint()->GetPosition().z();
  //std::cout << zpos << std::endl;
  int count = trajectory->GetPointEntries();
  std::cout << trajectory->GetPointEntries() << std::endl;
  if(trajectory->GetPoint(count-1)->GetPosition().z() < 250.){
    //std::cout << "Killed Event " << events << ", Pos:" << track->GetVertexPosition().z() << std::endl;
    //track->SetTrackStatus(fKillTrackAndSecondaries);
    G4RunManager::GetRunManager()->AbortEvent();
    return;
  } else {
    std::cout << "Not Eliminated" << std::endl;
  }
}*/

//double prezpos=0;
void EndZFilter::stepping(const G4Step* step) {
  // Get the track associated with this step.
  auto track{step->GetTrack()};
  if (auto region{
          track->GetVolume()->GetLogicalVolume()->GetRegion()->GetName()};
      region.compareTo("em_calorimeters_back") != 0)
    return;

  if (auto volume{track->GetNextVolume()->GetName()};
      volume.compareTo("recoil_PV") == 0
      or volume.compareTo("World_PV") == 0) {
    // If the recoil electron
    if (track->GetMomentum().mag() >= recoilMaxPThreshold_) {
      track->SetTrackStatus(fKillTrackAndSecondaries);
      G4RunManager::GetRunManager()->AbortEvent();
      return;
    }

    // Get the electron secondries
    bool hasBremCandidate = false;
    if (auto secondaries = step->GetSecondary(); secondaries->size() == 0) {
      track->SetTrackStatus(fKillTrackAndSecondaries);
      G4RunManager::GetRunManager()->AbortEvent();
      return;
    } else {
      for (auto& secondary_track : *secondaries) {
        G4String processName =
            secondary_track->GetCreatorProcess()->GetProcessName();

        if (processName.compareTo("eBrem") == 0 &&
            secondary_track->GetKineticEnergy() > bremEnergyThreshold_) {
          auto trackInfo{simcore::UserTrackInformation::get(secondary_track)};
          trackInfo->tagBremCandidate();

          getEventInfo()->incBremCandidateCount();

          hasBremCandidate = true;
        }
      }
    }

    if (!hasBremCandidate) {
      track->SetTrackStatus(fKillTrackAndSecondaries);
      G4RunManager::GetRunManager()->AbortEvent();
      return;
    }

    if (killRecoil_)
      track->SetTrackStatus(fStopAndKill);
    else
      track->SetTrackStatus(fSuspend);

  } else if (step->GetPostStepPoint()->GetKineticEnergy() == 0) {
    track->SetTrackStatus(fKillTrackAndSecondaries);
    G4RunManager::GetRunManager()->AbortEvent();
    return;
  }
  //track->GetStep()
  /*if(step->IsFirstStepInVolume()){
    prezpos = step->GetPreStepPoint()->GetPosition().z();
  }
  std::cout << "getPos:" << track->GetPosition().z() << std::endl;
  events++;
  std::cout << endZ_ << std::endl;
  double zpos = track->GetStep()->GetPostStepPoint()->GetPosition().z() ;//- track->GetStep()->GetPreStepPoint()->GetPosition().z();
  std::cout << zpos << std::endl;
  int count = trajectory->GetPointEntries();
  std::cout << trajectory->GetPointEntries() << std::endl;
  if(zpos-prezpos < 250.){
    //std::cout << "Killed Event " << events << ", Pos:" << track->GetVertexPosition().z() << std::endl;
    //track->SetTrackStatus(fKillTrackAndSecondaries);
    G4RunManager::GetRunManager()->AbortEvent();
    return;
  } else {
    std::cout << "Not Eliminated" << std::endl;
  }*/
}

void EndZFilter::EndOfEventAction(const G4Event* event) {
}
}  // namespace biasing


DECLARE_ACTION(biasing, EndZFilter)
