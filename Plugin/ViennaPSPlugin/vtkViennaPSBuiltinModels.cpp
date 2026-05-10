#include "vtkViennaPSBuiltinModels.h"

void vtkViennaPSModelRegistry::initializeBuiltinModels(bool geometry) {
    if (geometry) {
        ViennaPSModels::initializeGeometryModels();
    } else {
        ViennaPSModels::initializeEtchingModels();
        ViennaPSModels::initializeDepositionModels();
        ViennaPSModels::initializeDistributionModels();
    }
}
