#include "vtkViennaPSBuiltinModels.h"

#include <csDenseCellSet.hpp>
#include <models/psCF4O2Etching.hpp>
#include <models/psCF4O2Parameters.hpp>
#include <models/psFaradayCageEtching.hpp>
#include <models/psFluorocarbonEtching.hpp>
#include <models/psHBrO2Etching.hpp>
#include <models/psIonBeamEtching.hpp>
#include <models/psIonBeamParameters.hpp>
#include <models/psPlasmaEtchingParameters.hpp>
#include <models/psSF6C4F8Etching.hpp>
#include <models/psSF6O2Etching.hpp>
#include <models/psWetEtching.hpp>
#include <psConstants.hpp>
#include <psUnits.hpp>
#include <vcVectorType.hpp>

#include <iostream>
#include <limits>

namespace {
using NumericType = ViennaPSMeta::NumericType;
constexpr int D = ViennaPSMeta::D;

void registerWetEtchingProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata wetEtchingProcess;
  wetEtchingProcess.className = "WetEtching";
  wetEtchingProcess.displayName = "Wet Etching";
  wetEtchingProcess.description =
      "Anisotropic wet etching model for crystalline materials (e.g., Si in "
      "KOH/TMAH)";
  wetEtchingProcess.type = ModelType::EMULATION;

  ParameterMetadata etchMaterialsParam;
  etchMaterialsParam.name = "EtchMaterials";
  etchMaterialsParam.displayName = "Materials to Etch";
  etchMaterialsParam.documentation = "Select materials that will be etched";
  etchMaterialsParam.type = ParameterType::MATERIAL_LIST;
  etchMaterialsParam.defaultValue = MaterialListValue{{0}};
  etchMaterialsParam.category = ParameterCategory::BASIC;
  etchMaterialsParam.required = true;

  auto materialNames = getAllMaterialNames();
  etchMaterialsParam.enumOptions = materialNames;
  for (size_t i = 0; i < materialNames.size(); ++i) {
    etchMaterialsParam.materialMap[static_cast<int>(i)] = materialNames[i];
  }
  wetEtchingProcess.parameters.push_back(etchMaterialsParam);

  ParameterMetadata materialRateParam;
  materialRateParam.name = "MaterialRateMultiplier";
  materialRateParam.displayName = "Material Rate Multiplier";
  materialRateParam.documentation =
      "Rate multiplier for selected materials (1.0 = default rate)";
  materialRateParam.type = ParameterType::DOUBLE;
  materialRateParam.defaultValue = 1.0;
  materialRateParam.minValue = 0.0;
  materialRateParam.maxValue = 10.0;
  materialRateParam.category = ParameterCategory::BASIC;
  materialRateParam.unit = "";
  materialRateParam.stepSize = 0.1;
  materialRateParam.required = true;
  wetEtchingProcess.parameters.push_back(materialRateParam);

  ParameterMetadata r100Param;
  r100Param.name = "R100";
  r100Param.displayName = "(100) Plane Etch Rate";
  r100Param.documentation = "Etch rate for (100) crystal planes";
  r100Param.type = ParameterType::DOUBLE;
  r100Param.defaultValue = 0.0166666666667;
  r100Param.minValue = 0.0;
  r100Param.maxValue = 1.0;
  r100Param.category = ParameterCategory::BASIC;
  r100Param.unit = "";
  r100Param.stepSize = 0.001;
  r100Param.required = true;
  wetEtchingProcess.parameters.push_back(r100Param);

  ParameterMetadata r110Param;
  r110Param.name = "R110";
  r110Param.displayName = "(110) Plane Etch Rate";
  r110Param.documentation = "Etch rate for (110) crystal planes";
  r110Param.type = ParameterType::DOUBLE;
  r110Param.defaultValue = 0.0309166666667;
  r110Param.minValue = 0.0;
  r110Param.maxValue = 1.0;
  r110Param.category = ParameterCategory::BASIC;
  r110Param.unit = "";
  r110Param.stepSize = 0.001;
  r110Param.required = true;
  wetEtchingProcess.parameters.push_back(r110Param);

  ParameterMetadata r111Param;
  r111Param.name = "R111";
  r111Param.displayName = "(111) Plane Etch Rate";
  r111Param.documentation = "Etch rate for (111) crystal planes";
  r111Param.type = ParameterType::DOUBLE;
  r111Param.defaultValue = 0.000121666666667;
  r111Param.minValue = 0.0;
  r111Param.maxValue = 1.0;
  r111Param.category = ParameterCategory::BASIC;
  r111Param.unit = "";
  r111Param.stepSize = 0.0001;
  r111Param.required = true;
  wetEtchingProcess.parameters.push_back(r111Param);

  ParameterMetadata r311Param;
  r311Param.name = "R311";
  r311Param.displayName = "(311) Plane Etch Rate";
  r311Param.documentation = "Etch rate for (311) crystal planes";
  r311Param.type = ParameterType::DOUBLE;
  r311Param.defaultValue = 0.0300166666667;
  r311Param.minValue = 0.0;
  r311Param.maxValue = 1.0;
  r311Param.category = ParameterCategory::BASIC;
  r311Param.unit = "";
  r311Param.stepSize = 0.001;
  r311Param.required = true;
  wetEtchingProcess.parameters.push_back(r311Param);

  ParameterMetadata dir100XParam;
  dir100XParam.name = "Direction100_X";
  dir100XParam.displayName = "[100] Direction X";
  dir100XParam.documentation = "X component of [100] crystal direction";
  dir100XParam.type = ParameterType::DOUBLE;
  dir100XParam.defaultValue = 0.707106781187;
  dir100XParam.minValue = -1.0;
  dir100XParam.maxValue = 1.0;
  dir100XParam.category = ParameterCategory::ADVANCED;
  dir100XParam.unit = "";
  dir100XParam.stepSize = 0.01;
  dir100XParam.required = false;
  wetEtchingProcess.parameters.push_back(dir100XParam);

  ParameterMetadata dir100YParam;
  dir100YParam.name = "Direction100_Y";
  dir100YParam.displayName = "[100] Direction Y";
  dir100YParam.documentation = "Y component of [100] crystal direction";
  dir100YParam.type = ParameterType::DOUBLE;
  dir100YParam.defaultValue = 0.707106781187;
  dir100YParam.minValue = -1.0;
  dir100YParam.maxValue = 1.0;
  dir100YParam.category = ParameterCategory::ADVANCED;
  dir100YParam.unit = "";
  dir100YParam.stepSize = 0.01;
  dir100YParam.required = false;
  wetEtchingProcess.parameters.push_back(dir100YParam);

  ParameterMetadata dir100ZParam;
  dir100ZParam.name = "Direction100_Z";
  dir100ZParam.displayName = "[100] Direction Z";
  dir100ZParam.documentation = "Z component of [100] crystal direction";
  dir100ZParam.type = ParameterType::DOUBLE;
  dir100ZParam.defaultValue = 0.0;
  dir100ZParam.minValue = -1.0;
  dir100ZParam.maxValue = 1.0;
  dir100ZParam.category = ParameterCategory::ADVANCED;
  dir100ZParam.unit = "";
  dir100ZParam.stepSize = 0.01;
  dir100ZParam.required = false;
  wetEtchingProcess.parameters.push_back(dir100ZParam);

  ParameterMetadata dir010XParam;
  dir010XParam.name = "Direction010_X";
  dir010XParam.displayName = "[010] Direction X";
  dir010XParam.documentation = "X component of [010] crystal direction";
  dir010XParam.type = ParameterType::DOUBLE;
  dir010XParam.defaultValue = -0.707106781187;
  dir010XParam.minValue = -1.0;
  dir010XParam.maxValue = 1.0;
  dir010XParam.category = ParameterCategory::ADVANCED;
  dir010XParam.unit = "";
  dir010XParam.stepSize = 0.01;
  dir010XParam.required = false;
  wetEtchingProcess.parameters.push_back(dir010XParam);

  ParameterMetadata dir010YParam;
  dir010YParam.name = "Direction010_Y";
  dir010YParam.displayName = "[010] Direction Y";
  dir010YParam.documentation = "Y component of [010] crystal direction";
  dir010YParam.type = ParameterType::DOUBLE;
  dir010YParam.defaultValue = 0.707106781187;
  dir010YParam.minValue = -1.0;
  dir010YParam.maxValue = 1.0;
  dir010YParam.category = ParameterCategory::ADVANCED;
  dir010YParam.unit = "";
  dir010YParam.stepSize = 0.01;
  dir010YParam.required = false;
  wetEtchingProcess.parameters.push_back(dir010YParam);

  ParameterMetadata dir010ZParam;
  dir010ZParam.name = "Direction010_Z";
  dir010ZParam.displayName = "[010] Direction Z";
  dir010ZParam.documentation = "Z component of [010] crystal direction";
  dir010ZParam.type = ParameterType::DOUBLE;
  dir010ZParam.defaultValue = 0.0;
  dir010ZParam.minValue = -1.0;
  dir010ZParam.maxValue = 1.0;
  dir010ZParam.category = ParameterCategory::ADVANCED;
  dir010ZParam.unit = "";
  dir010ZParam.stepSize = 0.01;
  dir010ZParam.required = false;
  wetEtchingProcess.parameters.push_back(dir010ZParam);

  auto factory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                    vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 100.0);
    double materialRateMultiplier =
        registry.getParameter<double>(params, "MaterialRateMultiplier", 1.0);
    double r100 =
        registry.getParameter<double>(params, "R100", 0.0166666666667);
    double r110 =
        registry.getParameter<double>(params, "R110", 0.0309166666667);
    double r111 =
        registry.getParameter<double>(params, "R111", 0.000121666666667);
    double r311 =
        registry.getParameter<double>(params, "R311", 0.0300166666667);

    double dir100_x =
        registry.getParameter<double>(params, "Direction100_X", 0.707106781187);
    double dir100_y =
        registry.getParameter<double>(params, "Direction100_Y", 0.707106781187);
    double dir100_z =
        registry.getParameter<double>(params, "Direction100_Z", 0.0);

    double dir010_x = registry.getParameter<double>(params, "Direction010_X",
                                                    -0.707106781187);
    double dir010_y =
        registry.getParameter<double>(params, "Direction010_Y", 0.707106781187);
    double dir010_z =
        registry.getParameter<double>(params, "Direction010_Z", 0.0);

    viennacore::Vec3D<NumericType> direction100 = {dir100_x, dir100_y,
                                                   dir100_z};
    viennacore::Vec3D<NumericType> direction010 = {dir010_x, dir010_y,
                                                   dir010_z};

    std::vector<std::pair<viennaps::Material, NumericType>> materialRates;
    if (params.find("EtchMaterials") != params.end()) {
      auto matList = registry.getParameter<MaterialListValue>(
          params, "EtchMaterials", MaterialListValue{{0}});

      for (int matId : matList.materialIds) {
        auto material = static_cast<viennaps::Material>(matId);
        if (material != viennaps::Material::Undefined) {
          materialRates.push_back({material, materialRateMultiplier});
        }
      }
    } else {
      materialRates.push_back({viennaps::Material::Si, materialRateMultiplier});
    }

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;

          auto model = viennaps::SmartPointer<
              viennaps::WetEtching<NumericType, Dim>>::New(direction100,
                                                           direction010, r100,
                                                           r110, r111, r311,
                                                           materialRates);

          VPSLOG_DEBUG(nullptr, "Crystal orientation rates:");
          VPSLOG_DEBUG(nullptr, "  (100): ", r100, " µm/s");
          VPSLOG_DEBUG(nullptr, "  (110): ", r110, " µm/s");
          VPSLOG_DEBUG(nullptr, "  (111): ", r111, " µm/s");
          VPSLOG_DEBUG(nullptr, "  (311): ", r311, " µm/s");
          VPSLOG_DEBUG(nullptr, "Materials to etch: ", materialRates.size());
          for (const auto &mat : materialRates) {
            VPSLOG_DEBUG(nullptr, "  ",
                         viennaps::MaterialMap::toString(mat.first),
                         " (rate multiplier: ", mat.second, ")");
          }
          VPSLOG_DEBUG(nullptr, "Process time: ", processTime, " s");

          viennaps::Process<NumericType, Dim> process;
          process.setDomain(psDomain);
          process.setProcessModel(model);

#ifndef MULTI_STEP
          process.setProcessDuration(processTime);
          process.apply();
#else
                const int numSteps = 20;
                const double stepTime = processTime / numSteps;

                process.setProcessDuration(stepTime);
                for (int step = 0; step < numSteps; ++step) {
                    process.apply();
                }
#endif

          ViennaPSModels::convertToVTK<Dim>(psDomain, output, params);
        });
  };

  vtkViennaPSModelRegistry::getInstance().registerProcessModel(
      "WetEtching", wetEtchingProcess, factory);
}

void registerSF6O2EtchingProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata sf6o2Process;
  sf6o2Process.className = "SF6O2Etching";
  sf6o2Process.displayName = "SF6/O2 Plasma Etching";
  sf6o2Process.description =
      "SF6/O2 plasma etching model for Si with oxygen passivation";
  sf6o2Process.type = ModelType::SIMULATION;

  ParameterMetadata ionFluxParam;
  ionFluxParam.name = "IonFlux";
  ionFluxParam.displayName = "Ion Flux";
  ionFluxParam.documentation = "Ion flux (1e15 /cm²/s)";
  ionFluxParam.type = ParameterType::DOUBLE;
  ionFluxParam.defaultValue = 12.0;
  ionFluxParam.minValue = 0.0;
    ionFluxParam.maxValue = std::numeric_limits<double>::max();
  ionFluxParam.category = ParameterCategory::BASIC;
  ionFluxParam.unit = "";
  ionFluxParam.stepSize = 1.0;
  ionFluxParam.required = true;
  sf6o2Process.parameters.push_back(ionFluxParam);

  ParameterMetadata etchantFluxParam;
  etchantFluxParam.name = "EtchantFlux";
  etchantFluxParam.displayName = "Etchant (SF6) Flux";
  etchantFluxParam.documentation = "SF6 etchant flux (1e15 /cm²/s)";
  etchantFluxParam.type = ParameterType::DOUBLE;
  etchantFluxParam.defaultValue = 1800.0;
  etchantFluxParam.minValue = 0.0;
    etchantFluxParam.maxValue = std::numeric_limits<double>::max();
  etchantFluxParam.category = ParameterCategory::BASIC;
  etchantFluxParam.unit = "";
  etchantFluxParam.stepSize = 100.0;
  etchantFluxParam.required = true;
  sf6o2Process.parameters.push_back(etchantFluxParam);

  ParameterMetadata oxygenFluxParam;
  oxygenFluxParam.name = "OxygenFlux";
  oxygenFluxParam.displayName = "Oxygen Flux";
  oxygenFluxParam.documentation = "Oxygen passivation flux (1e15 /cm²/s)";
  oxygenFluxParam.type = ParameterType::DOUBLE;
  oxygenFluxParam.defaultValue = 100.0;
  oxygenFluxParam.minValue = 0.0;
    oxygenFluxParam.maxValue = std::numeric_limits<double>::max();
  oxygenFluxParam.category = ParameterCategory::BASIC;
  oxygenFluxParam.unit = "";
  oxygenFluxParam.stepSize = 10.0;
  oxygenFluxParam.required = true;
  sf6o2Process.parameters.push_back(oxygenFluxParam);

  ParameterMetadata meanEnergyParam;
  meanEnergyParam.name = "MeanEnergy";
  meanEnergyParam.displayName = "Mean Ion Energy";
  meanEnergyParam.documentation = "Mean energy of ions";
  meanEnergyParam.type = ParameterType::DOUBLE;
  meanEnergyParam.defaultValue = 100.0;
  meanEnergyParam.minValue = 10.0;
  meanEnergyParam.maxValue = 500.0;
  meanEnergyParam.category = ParameterCategory::BASIC;
  meanEnergyParam.unit = "";
  meanEnergyParam.stepSize = 10.0;
  meanEnergyParam.required = true;
  sf6o2Process.parameters.push_back(meanEnergyParam);

  ParameterMetadata sigmaEnergyParam;
  sigmaEnergyParam.name = "SigmaEnergy";
  sigmaEnergyParam.displayName = "Ion Energy Sigma";
  sigmaEnergyParam.documentation =
      "Standard deviation of ion energy distribution";
  sigmaEnergyParam.type = ParameterType::DOUBLE;
  sigmaEnergyParam.defaultValue = 10.0;
  sigmaEnergyParam.minValue = 0.1;
  sigmaEnergyParam.maxValue = 100.0;
  sigmaEnergyParam.category = ParameterCategory::BASIC;
  sigmaEnergyParam.unit = "";
  sigmaEnergyParam.stepSize = 1.0;
  sigmaEnergyParam.required = true;
  sf6o2Process.parameters.push_back(sigmaEnergyParam);

  ParameterMetadata ionExponentParam;
  ionExponentParam.name = "IonExponent";
  ionExponentParam.displayName = "Ion Angular Distribution";
  ionExponentParam.documentation = "Exponent for ion angular distribution";
  ionExponentParam.type = ParameterType::DOUBLE;
  ionExponentParam.defaultValue = 500.0;
  ionExponentParam.minValue = 1.0;
  ionExponentParam.maxValue = 1000.0;
  ionExponentParam.category = ParameterCategory::ADVANCED;
  ionExponentParam.unit = "";
  ionExponentParam.stepSize = 50.0;
  ionExponentParam.required = false;
  sf6o2Process.parameters.push_back(ionExponentParam);

  ParameterMetadata oxySputterParam;
  oxySputterParam.name = "OxygenSputterYield";
  oxySputterParam.displayName = "Oxygen Sputter Yield";
  oxySputterParam.documentation = "Ion-enhanced oxygen sputtering yield";
  oxySputterParam.type = ParameterType::DOUBLE;
  oxySputterParam.defaultValue = 3.0;
  oxySputterParam.minValue = 0.0;
  oxySputterParam.maxValue = 10.0;
  oxySputterParam.category = ParameterCategory::ADVANCED;
  oxySputterParam.unit = "";
  oxySputterParam.stepSize = 0.5;
  oxySputterParam.required = false;
  sf6o2Process.parameters.push_back(oxySputterParam);

  ParameterMetadata etchStopParam;
  etchStopParam.name = "EtchStopDepth";
  etchStopParam.displayName = "Etch Stop Depth";
  etchStopParam.documentation =
      "Z-coordinate to stop etching. Leave at minimum for no stop.";
  etchStopParam.type = ParameterType::DOUBLE;
  etchStopParam.defaultValue = std::numeric_limits<double>::lowest();
  etchStopParam.minValue = std::numeric_limits<double>::lowest();
  etchStopParam.maxValue = 0.0;
  etchStopParam.category = ParameterCategory::ADVANCED;
  etchStopParam.unit = "";
  etchStopParam.stepSize = 10.0;
  etchStopParam.required = false;
  sf6o2Process.parameters.push_back(etchStopParam);

  ParameterMetadata betaEParam;
  betaEParam.name = "BetaE";
  betaEParam.displayName = "Etchant Sticking Coefficient";
  betaEParam.documentation = "Sticking probability of etchant species";
  betaEParam.type = ParameterType::DOUBLE;
  betaEParam.defaultValue = 0.7;
  betaEParam.minValue = 0.0;
  betaEParam.maxValue = 1.0;
  betaEParam.category = ParameterCategory::ADVANCED;
  betaEParam.unit = "";
  betaEParam.stepSize = 0.05;
  betaEParam.required = false;
  sf6o2Process.parameters.push_back(betaEParam);

  ParameterMetadata betaPParam;
  betaPParam.name = "BetaP";
  betaPParam.displayName = "Passivation Sticking Coefficient";
  betaPParam.documentation =
      "Sticking probability of passivation species (oxygen)";
  betaPParam.type = ParameterType::DOUBLE;
  betaPParam.defaultValue = 1.0;
  betaPParam.minValue = 0.0;
  betaPParam.maxValue = 1.0;
  betaPParam.category = ParameterCategory::ADVANCED;
  betaPParam.unit = "";
  betaPParam.stepSize = 0.05;
  betaPParam.required = false;
  sf6o2Process.parameters.push_back(betaPParam);

  ParameterMetadata kSigmaParam;
  kSigmaParam.name = "KSigma";
  kSigmaParam.displayName = "Chemical Etching Rate";
  kSigmaParam.documentation = "Chemical etching rate constant (1e15 cm⁻²s⁻¹)";
  kSigmaParam.type = ParameterType::DOUBLE;
  kSigmaParam.defaultValue = 300.0;
  kSigmaParam.minValue = 0.0;
  kSigmaParam.maxValue = 1000.0;
  kSigmaParam.category = ParameterCategory::ADVANCED;
  kSigmaParam.unit = "";
  kSigmaParam.stepSize = 10.0;
  kSigmaParam.required = false;
  sf6o2Process.parameters.push_back(kSigmaParam);

  ParameterMetadata lengthUnitParam;
  lengthUnitParam.name = "LengthUnit";
  lengthUnitParam.displayName = "Length Unit";
  lengthUnitParam.documentation = "Unit for length measurements";
  lengthUnitParam.type = ParameterType::ENUM;
  lengthUnitParam.defaultValue = 0;
  lengthUnitParam.category = ParameterCategory::BASIC;
  lengthUnitParam.required = true;
  lengthUnitParam.enumOptions = {"Nanometer",  "Micrometer", "Millimeter",
                                 "Centimeter", "Meter",      "Angstrom"};
  sf6o2Process.parameters.push_back(lengthUnitParam);

  ParameterMetadata timeUnitParam;
  timeUnitParam.name = "TimeUnit";
  timeUnitParam.displayName = "Time Unit";
  timeUnitParam.documentation = "Unit for time measurements";
  timeUnitParam.type = ParameterType::ENUM;
  timeUnitParam.defaultValue = 0;
  timeUnitParam.enumOptions = {"Second", "Minute", "Millisecond"};
  timeUnitParam.category = ParameterCategory::BASIC;
  timeUnitParam.required = true;
  sf6o2Process.parameters.push_back(timeUnitParam);

  auto factory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                    vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    int lengthUnit = registry.getParameter<int>(params, "LengthUnit", 0);
    int timeUnit = registry.getParameter<int>(params, "TimeUnit", 0);

    switch (lengthUnit) {
    case 0:
      viennaps::units::Length::setUnit(viennaps::units::Length::NANOMETER);
      break;
    case 1:
      viennaps::units::Length::setUnit(viennaps::units::Length::MICROMETER);
      break;
    case 2:
      viennaps::units::Length::setUnit(viennaps::units::Length::MILLIMETER);
      break;
    case 3:
      viennaps::units::Length::setUnit(viennaps::units::Length::CENTIMETER);
      break;
    case 4:
      viennaps::units::Length::setUnit(viennaps::units::Length::METER);
      break;
    case 5:
      viennaps::units::Length::setUnit(viennaps::units::Length::ANGSTROM);
      break;
    default:
      viennaps::units::Length::setUnit(viennaps::units::Length::NANOMETER);
      break;
    }

    switch (timeUnit) {
    case 0:
      viennaps::units::Time::setUnit(viennaps::units::Time::SECOND);
      break;
    case 1:
      viennaps::units::Time::setUnit(viennaps::units::Time::MINUTE);
      break;
    case 2:
      viennaps::units::Time::setUnit(viennaps::units::Time::MILLISECOND);
      break;
    default:
      viennaps::units::Time::setUnit(viennaps::units::Time::SECOND);
      break;
    }

    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 10.0);
    double ionFlux = registry.getParameter<double>(params, "IonFlux", 12.0);
    double etchantFlux =
        registry.getParameter<double>(params, "EtchantFlux", 1800.0);
    double oxygenFlux =
        registry.getParameter<double>(params, "OxygenFlux", 100.0);
    double meanEnergy =
        registry.getParameter<double>(params, "MeanEnergy", 100.0);
    double sigmaEnergy =
        registry.getParameter<double>(params, "SigmaEnergy", 10.0);
    double ionExponent =
        registry.getParameter<double>(params, "IonExponent", 500.0);
    double oxySputterYield =
        registry.getParameter<double>(params, "OxygenSputterYield", 3.0);
    double etchStopDepth = registry.getParameter<double>(
        params, "EtchStopDepth", std::numeric_limits<NumericType>::lowest());
    double betaE = registry.getParameter<double>(params, "BetaE", 0.7);
    double betaP = registry.getParameter<double>(params, "BetaP", 1.0);
    double kSigma = registry.getParameter<double>(params, "KSigma", 300.0);

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;

          auto sf6o2Params =
              viennaps::SF6O2Etching<NumericType, Dim>::defaultParameters();
          sf6o2Params.ionFlux = ionFlux;
          sf6o2Params.etchantFlux = etchantFlux;
          sf6o2Params.passivationFlux = oxygenFlux;
          sf6o2Params.Ions.meanEnergy = meanEnergy;
          sf6o2Params.Ions.sigmaEnergy = sigmaEnergy;
          sf6o2Params.Ions.exponent = ionExponent;
          sf6o2Params.Passivation.A_ie = oxySputterYield;
          sf6o2Params.etchStopDepth = etchStopDepth;
          sf6o2Params.Substrate.k_sigma = kSigma;

          sf6o2Params.beta_E.set(viennaps::Material::Si, betaE);
          sf6o2Params.beta_E.set(viennaps::Material::Mask, betaE);
          sf6o2Params.beta_P.set(viennaps::Material::Si, betaP);
          sf6o2Params.beta_P.set(viennaps::Material::Mask, betaP);

          auto model = viennaps::SmartPointer<
              viennaps::SF6O2Etching<NumericType, Dim>>::New(sf6o2Params);

          VPSLOG_DEBUG(nullptr, "Ion flux: ", ionFlux, " (1e15 /cm²/s)");
          VPSLOG_DEBUG(nullptr, "Etchant flux: ", etchantFlux,
                       " (1e15 /cm²/s)");
          VPSLOG_DEBUG(nullptr, "Oxygen flux: ", oxygenFlux, " (1e15 /cm²/s)");
          VPSLOG_DEBUG(nullptr, "Mean ion energy: ", meanEnergy, " eV");
          VPSLOG_DEBUG(nullptr, "Ion energy sigma: ", sigmaEnergy, " eV");
          if (etchStopDepth > std::numeric_limits<NumericType>::lowest()) {
            VPSLOG_DEBUG(nullptr, "Etch stop depth: ", etchStopDepth, " nm");
          }

          viennaps::Process<NumericType, Dim> process;
          process.setDomain(psDomain);
          process.setProcessModel(model);
          {
            viennaps::RayTracingParameters rayParams;
            rayParams.raysPerPoint = static_cast<unsigned>(
                registry.getParameter<int>(params, "NumRaysPerPoint", 1000));
            process.setParameters(rayParams);
          }

#ifndef MULTI_STEP
          process.setProcessDuration(processTime);
          process.apply();
#else
                const int numSteps = 20;
                const double stepTime = processTime / numSteps;

                process.setProcessDuration(stepTime);
                for (int step = 0; step < numSteps; ++step) {
                    process.apply();
                }
#endif

          ViennaPSModels::convertToVTK<Dim>(psDomain, output, params);
        });
  };

  sf6o2Process.parameters.push_back(makeNumRaysPerPointParam());

  vtkViennaPSModelRegistry::getInstance().registerProcessModel(
      "SF6O2Etching", sf6o2Process, factory);
}

void registerSF6C4F8EtchingProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata sf6c4f8Process;
  sf6c4f8Process.className = "SF6C4F8Etching";
  sf6c4f8Process.displayName = "SF6/C4F8 Plasma Etching";
  sf6c4f8Process.description = "SF6/C4F8 plasma etching model for Si with "
                               "polymer layer etching (no passivation)";
  sf6c4f8Process.type = ModelType::SIMULATION;

  ParameterMetadata ionFluxParam;
  ionFluxParam.name = "IonFlux";
  ionFluxParam.displayName = "Ion Flux";
  ionFluxParam.documentation = "Ion flux (1e15 /cm²/s)";
  ionFluxParam.type = ParameterType::DOUBLE;
  ionFluxParam.defaultValue = 12.0;
  ionFluxParam.minValue = 0.0;
  ionFluxParam.maxValue = 100.0;
  ionFluxParam.category = ParameterCategory::BASIC;
  ionFluxParam.unit = "";
  ionFluxParam.stepSize = 1.0;
  ionFluxParam.required = true;
  sf6c4f8Process.parameters.push_back(ionFluxParam);

  ParameterMetadata etchantFluxParam;
  etchantFluxParam.name = "EtchantFlux";
  etchantFluxParam.displayName = "Etchant (SF6) Flux";
  etchantFluxParam.documentation = "SF6 etchant flux (1e15 /cm²/s)";
  etchantFluxParam.type = ParameterType::DOUBLE;
  etchantFluxParam.defaultValue = 1800.0;
  etchantFluxParam.minValue = 0.0;
  etchantFluxParam.maxValue = 10000.0;
  etchantFluxParam.category = ParameterCategory::BASIC;
  etchantFluxParam.unit = "";
  etchantFluxParam.stepSize = 100.0;
  etchantFluxParam.required = true;
  sf6c4f8Process.parameters.push_back(etchantFluxParam);

  ParameterMetadata meanEnergyParam;
  meanEnergyParam.name = "MeanEnergy";
  meanEnergyParam.displayName = "Mean Ion Energy";
  meanEnergyParam.documentation = "Mean energy of ions";
  meanEnergyParam.type = ParameterType::DOUBLE;
  meanEnergyParam.defaultValue = 100.0;
  meanEnergyParam.minValue = 10.0;
  meanEnergyParam.maxValue = 500.0;
  meanEnergyParam.category = ParameterCategory::BASIC;
  meanEnergyParam.unit = "";
  meanEnergyParam.stepSize = 10.0;
  meanEnergyParam.required = true;
  sf6c4f8Process.parameters.push_back(meanEnergyParam);

  ParameterMetadata sigmaEnergyParam;
  sigmaEnergyParam.name = "SigmaEnergy";
  sigmaEnergyParam.displayName = "Ion Energy Sigma";
  sigmaEnergyParam.documentation =
      "Standard deviation of ion energy distribution";
  sigmaEnergyParam.type = ParameterType::DOUBLE;
  sigmaEnergyParam.defaultValue = 10.0;
  sigmaEnergyParam.minValue = 0.1;
  sigmaEnergyParam.maxValue = 100.0;
  sigmaEnergyParam.category = ParameterCategory::BASIC;
  sigmaEnergyParam.unit = "";
  sigmaEnergyParam.stepSize = 1.0;
  sigmaEnergyParam.required = true;
  sf6c4f8Process.parameters.push_back(sigmaEnergyParam);

  ParameterMetadata ionExponentParam;
  ionExponentParam.name = "IonExponent";
  ionExponentParam.displayName = "Ion Angular Distribution";
  ionExponentParam.documentation = "Exponent for ion angular distribution";
  ionExponentParam.type = ParameterType::DOUBLE;
  ionExponentParam.defaultValue = 500.0;
  ionExponentParam.minValue = 1.0;
  ionExponentParam.maxValue = 1000.0;
  ionExponentParam.category = ParameterCategory::ADVANCED;
  ionExponentParam.unit = "";
  ionExponentParam.stepSize = 50.0;
  ionExponentParam.required = false;
  sf6c4f8Process.parameters.push_back(ionExponentParam);

  ParameterMetadata etchStopParam;
  etchStopParam.name = "EtchStopDepth";
  etchStopParam.displayName = "Etch Stop Depth";
  etchStopParam.documentation =
      "Z-coordinate to stop etching. Leave at minimum for no stop.";
  etchStopParam.type = ParameterType::DOUBLE;
  etchStopParam.defaultValue = std::numeric_limits<double>::lowest();
  etchStopParam.minValue = std::numeric_limits<double>::lowest();
  etchStopParam.maxValue = 0.0;
  etchStopParam.category = ParameterCategory::ADVANCED;
  etchStopParam.unit = "";
  etchStopParam.stepSize = 10.0;
  etchStopParam.required = false;
  sf6c4f8Process.parameters.push_back(etchStopParam);

  ParameterMetadata betaEParam;
  betaEParam.name = "BetaE";
  betaEParam.displayName = "Etchant Sticking Coefficient";
  betaEParam.documentation = "Sticking probability of etchant species";
  betaEParam.type = ParameterType::DOUBLE;
  betaEParam.defaultValue = 0.7;
  betaEParam.minValue = 0.0;
  betaEParam.maxValue = 1.0;
  betaEParam.category = ParameterCategory::ADVANCED;
  betaEParam.unit = "";
  betaEParam.stepSize = 0.05;
  betaEParam.required = false;
  sf6c4f8Process.parameters.push_back(betaEParam);

  ParameterMetadata kSigmaParam;
  kSigmaParam.name = "KSigma";
  kSigmaParam.displayName = "Chemical Etching Rate";
  kSigmaParam.documentation = "Chemical etching rate constant (1e15 cm⁻²s⁻¹)";
  kSigmaParam.type = ParameterType::DOUBLE;
  kSigmaParam.defaultValue = 300.0;
  kSigmaParam.minValue = 0.0;
  kSigmaParam.maxValue = 1000.0;
  kSigmaParam.category = ParameterCategory::ADVANCED;
  kSigmaParam.unit = "";
  kSigmaParam.stepSize = 10.0;
  kSigmaParam.required = false;
  sf6c4f8Process.parameters.push_back(kSigmaParam);

  ParameterMetadata polymerDensityParam;
  polymerDensityParam.name = "PolymerDensity";
  polymerDensityParam.displayName = "Polymer Layer Density";
  polymerDensityParam.documentation =
      "C4F8 polymer layer density (1e22 atoms/cm³)";
  polymerDensityParam.type = ParameterType::DOUBLE;
  polymerDensityParam.defaultValue = 5.0;
  polymerDensityParam.minValue = 1.0;
  polymerDensityParam.maxValue = 20.0;
  polymerDensityParam.category = ParameterCategory::ADVANCED;
  polymerDensityParam.unit = "";
  polymerDensityParam.stepSize = 0.5;
  polymerDensityParam.required = false;
  sf6c4f8Process.parameters.push_back(polymerDensityParam);

  ParameterMetadata polymerThresholdParam;
  polymerThresholdParam.name = "PolymerThresholdEnergy";
  polymerThresholdParam.displayName = "Polymer Sputter Threshold";
  polymerThresholdParam.documentation =
      "Threshold energy for polymer sputtering";
  polymerThresholdParam.type = ParameterType::DOUBLE;
  polymerThresholdParam.defaultValue = 15.0;
  polymerThresholdParam.minValue = 5.0;
  polymerThresholdParam.maxValue = 50.0;
  polymerThresholdParam.category = ParameterCategory::ADVANCED;
  polymerThresholdParam.unit = "";
  polymerThresholdParam.stepSize = 1.0;
  polymerThresholdParam.required = false;
  sf6c4f8Process.parameters.push_back(polymerThresholdParam);

  ParameterMetadata lengthUnitParam;
  lengthUnitParam.name = "LengthUnit";
  lengthUnitParam.displayName = "Length Unit";
  lengthUnitParam.documentation = "Unit for length measurements";
  lengthUnitParam.type = ParameterType::ENUM;
  lengthUnitParam.defaultValue = 0;
  lengthUnitParam.category = ParameterCategory::BASIC;
  lengthUnitParam.required = true;
  lengthUnitParam.enumOptions = {"Nanometer",  "Micrometer", "Millimeter",
                                 "Centimeter", "Meter",      "Angstrom"};
  sf6c4f8Process.parameters.push_back(lengthUnitParam);

  ParameterMetadata timeUnitParam;
  timeUnitParam.name = "TimeUnit";
  timeUnitParam.displayName = "Time Unit";
  timeUnitParam.documentation = "Unit for time measurements";
  timeUnitParam.type = ParameterType::ENUM;
  timeUnitParam.defaultValue = 0;
  timeUnitParam.enumOptions = {"Second", "Minute", "Millisecond"};
  timeUnitParam.category = ParameterCategory::BASIC;
  timeUnitParam.required = true;
  sf6c4f8Process.parameters.push_back(timeUnitParam);

  auto factory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                    vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    int lengthUnit = registry.getParameter<int>(params, "LengthUnit", 0);
    int timeUnit = registry.getParameter<int>(params, "TimeUnit", 0);

    switch (lengthUnit) {
    case 0:
      viennaps::units::Length::setUnit(viennaps::units::Length::NANOMETER);
      break;
    case 1:
      viennaps::units::Length::setUnit(viennaps::units::Length::MICROMETER);
      break;
    case 2:
      viennaps::units::Length::setUnit(viennaps::units::Length::MILLIMETER);
      break;
    case 3:
      viennaps::units::Length::setUnit(viennaps::units::Length::CENTIMETER);
      break;
    case 4:
      viennaps::units::Length::setUnit(viennaps::units::Length::METER);
      break;
    case 5:
      viennaps::units::Length::setUnit(viennaps::units::Length::ANGSTROM);
      break;
    default:
      viennaps::units::Length::setUnit(viennaps::units::Length::NANOMETER);
      break;
    }

    switch (timeUnit) {
    case 0:
      viennaps::units::Time::setUnit(viennaps::units::Time::SECOND);
      break;
    case 1:
      viennaps::units::Time::setUnit(viennaps::units::Time::MINUTE);
      break;
    case 2:
      viennaps::units::Time::setUnit(viennaps::units::Time::MILLISECOND);
      break;
    default:
      viennaps::units::Time::setUnit(viennaps::units::Time::SECOND);
      break;
    }

    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 10.0);
    double ionFlux = registry.getParameter<double>(params, "IonFlux", 12.0);
    double etchantFlux =
        registry.getParameter<double>(params, "EtchantFlux", 1800.0);
    double meanEnergy =
        registry.getParameter<double>(params, "MeanEnergy", 100.0);
    double sigmaEnergy =
        registry.getParameter<double>(params, "SigmaEnergy", 10.0);
    double ionExponent =
        registry.getParameter<double>(params, "IonExponent", 500.0);
    double etchStopDepth = registry.getParameter<double>(
        params, "EtchStopDepth", std::numeric_limits<NumericType>::lowest());
    double betaE = registry.getParameter<double>(params, "BetaE", 0.7);
    double kSigma = registry.getParameter<double>(params, "KSigma", 300.0);
    double polymerDensity =
        registry.getParameter<double>(params, "PolymerDensity", 5.0);
    double polymerThreshold =
        registry.getParameter<double>(params, "PolymerThresholdEnergy", 15.0);

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;

          auto sf6c4f8Params =
              viennaps::SF6C4F8Etching<NumericType, Dim>::defaultParameters();
          sf6c4f8Params.ionFlux = ionFlux;
          sf6c4f8Params.etchantFlux = etchantFlux;
          sf6c4f8Params.passivationFlux = 0.0;
          sf6c4f8Params.Ions.meanEnergy = meanEnergy;
          sf6c4f8Params.Ions.sigmaEnergy = sigmaEnergy;
          sf6c4f8Params.Ions.exponent = ionExponent;
          sf6c4f8Params.etchStopDepth = etchStopDepth;
          sf6c4f8Params.Substrate.k_sigma = kSigma;

          sf6c4f8Params.Polymer.rho = polymerDensity;
          sf6c4f8Params.Polymer.Eth_sp = polymerThreshold;

          sf6c4f8Params.beta_E.set(viennaps::Material::Si, betaE);
          sf6c4f8Params.beta_E.set(viennaps::Material::Mask, betaE);

          auto model = viennaps::SmartPointer<
              viennaps::SF6C4F8Etching<NumericType, Dim>>::New(sf6c4f8Params);

          // C4F8 polymer pre-deposited, no passivation step
          VPSLOG_DEBUG(nullptr, "Ion flux: ", ionFlux, " (1e15 /cm²/s)");
          VPSLOG_DEBUG(nullptr, "Etchant flux: ", etchantFlux,
                       " (1e15 /cm²/s)");
          VPSLOG_DEBUG(nullptr, "Mean ion energy: ", meanEnergy, " eV");
          VPSLOG_DEBUG(nullptr, "Ion energy sigma: ", sigmaEnergy, " eV");
          if (etchStopDepth > std::numeric_limits<NumericType>::lowest()) {
            VPSLOG_DEBUG(nullptr, "Etch stop depth: ", etchStopDepth, " nm");
          }

          viennaps::Process<NumericType, Dim> process;
          process.setDomain(psDomain);
          process.setProcessModel(model);
          {
            viennaps::RayTracingParameters rayParams;
            rayParams.raysPerPoint = static_cast<unsigned>(
                registry.getParameter<int>(params, "NumRaysPerPoint", 1000));
            process.setParameters(rayParams);
          }

#ifndef MULTI_STEP
          process.setProcessDuration(processTime);
          process.apply();
#else
                const int numSteps = 20;
                const double stepTime = processTime / numSteps;

                process.setProcessDuration(stepTime);
                for (int step = 0; step < numSteps; ++step) {
                    process.apply();
                }
#endif

          ViennaPSModels::convertToVTK<Dim>(psDomain, output, params);
        });
  };

  sf6c4f8Process.parameters.push_back(makeNumRaysPerPointParam());

  vtkViennaPSModelRegistry::getInstance().registerProcessModel(
      "SF6C4F8Etching", sf6c4f8Process, factory);
}

void registerIonBeamEtchingProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata ionBeamProcess;
  ionBeamProcess.className = "IonBeamEtching";
  ionBeamProcess.displayName = "Ion Beam Etching";
  ionBeamProcess.description =
      "Physical ion beam etching with optional redeposition";
  ionBeamProcess.type = ModelType::SIMULATION;

  ParameterMetadata planeRateParam;
  planeRateParam.name = "PlaneWaferRate";
  planeRateParam.displayName = "Plane Wafer Rate";
  planeRateParam.documentation = "Etching rate on a flat wafer surface";
  planeRateParam.type = ParameterType::DOUBLE;
  planeRateParam.defaultValue = 1.0;
  planeRateParam.minValue = 0.0;
  planeRateParam.maxValue = 10.0;
  planeRateParam.category = ParameterCategory::BASIC;
  planeRateParam.unit = "";
  planeRateParam.stepSize = 0.1;
  planeRateParam.required = true;
  ionBeamProcess.parameters.push_back(planeRateParam);

  ParameterMetadata meanEnergyParam;
  meanEnergyParam.name = "MeanEnergy";
  meanEnergyParam.displayName = "Mean Ion Energy";
  meanEnergyParam.documentation = "Mean energy of ions";
  meanEnergyParam.type = ParameterType::DOUBLE;
  meanEnergyParam.defaultValue = 250.0;
  meanEnergyParam.minValue = 50.0;
  meanEnergyParam.maxValue = 1000.0;
  meanEnergyParam.category = ParameterCategory::BASIC;
  meanEnergyParam.unit = "";
  meanEnergyParam.stepSize = 10.0;
  meanEnergyParam.required = true;
  ionBeamProcess.parameters.push_back(meanEnergyParam);

  ParameterMetadata sigmaEnergyParam;
  sigmaEnergyParam.name = "SigmaEnergy";
  sigmaEnergyParam.displayName = "Ion Energy Sigma";
  sigmaEnergyParam.documentation =
      "Standard deviation of ion energy distribution";
  sigmaEnergyParam.type = ParameterType::DOUBLE;
  sigmaEnergyParam.defaultValue = 10.0;
  sigmaEnergyParam.minValue = 0.1;
  sigmaEnergyParam.maxValue = 100.0;
  sigmaEnergyParam.category = ParameterCategory::BASIC;
  sigmaEnergyParam.unit = "";
  sigmaEnergyParam.stepSize = 1.0;
  sigmaEnergyParam.required = true;
  ionBeamProcess.parameters.push_back(sigmaEnergyParam);

  ParameterMetadata thresholdParam;
  thresholdParam.name = "ThresholdEnergy";
  thresholdParam.displayName = "Threshold Energy";
  thresholdParam.documentation = "Minimum energy for etching";
  thresholdParam.type = ParameterType::DOUBLE;
  thresholdParam.defaultValue = 20.0;
  thresholdParam.minValue = 0.0;
  thresholdParam.maxValue = 100.0;
  thresholdParam.category = ParameterCategory::BASIC;
  thresholdParam.unit = "";
  thresholdParam.stepSize = 1.0;
  thresholdParam.required = true;
  ionBeamProcess.parameters.push_back(thresholdParam);

  ParameterMetadata tiltAngleParam;
  tiltAngleParam.name = "TiltAngle";
  tiltAngleParam.displayName = "Beam Tilt Angle";
  tiltAngleParam.documentation =
      "Ion beam tilt angle from surface normal (degrees)";
  tiltAngleParam.type = ParameterType::DOUBLE;
  tiltAngleParam.defaultValue = 0.0;
  tiltAngleParam.minValue = -89.0;
  tiltAngleParam.maxValue = 89.0;
  tiltAngleParam.category = ParameterCategory::BASIC;
  tiltAngleParam.unit = "°";
  tiltAngleParam.stepSize = 1.0;
  tiltAngleParam.required = true;
  ionBeamProcess.parameters.push_back(tiltAngleParam);

  ParameterMetadata maskParam;
  maskParam.name = "MaskMaterials";
  maskParam.displayName = "Materials to mask";
  maskParam.documentation = "Select mask materials that won't be etched";
  maskParam.type = ParameterType::MATERIAL_LIST;
  maskParam.defaultValue = MaterialListValue{{1}};
  maskParam.category = ParameterCategory::BASIC;
  maskParam.required = true;

  auto materialNames = getAllMaterialNames();
  maskParam.enumOptions = materialNames;
  for (size_t i = 0; i < materialNames.size(); ++i) {
    maskParam.materialMap[static_cast<int>(i)] = materialNames[i];
  }
  ionBeamProcess.parameters.push_back(maskParam);

  ParameterMetadata ionExponentParam;
  ionExponentParam.name = "IonExponent";
  ionExponentParam.displayName = "Ion Angular Distribution";
  ionExponentParam.documentation = "Exponent for ion angular distribution";
  ionExponentParam.type = ParameterType::DOUBLE;
  ionExponentParam.defaultValue = 100.0;
  ionExponentParam.minValue = 1.0;
  ionExponentParam.maxValue = 1000.0;
  ionExponentParam.category = ParameterCategory::ADVANCED;
  ionExponentParam.unit = "";
  ionExponentParam.stepSize = 10.0;
  ionExponentParam.required = false;
  ionBeamProcess.parameters.push_back(ionExponentParam);

  ParameterMetadata nlParam;
  nlParam.name = "NL";
  nlParam.displayName = "Reflection Parameter n_l";
  nlParam.documentation = "Parameter for energy reflection model";
  nlParam.type = ParameterType::DOUBLE;
  nlParam.defaultValue = 10.0;
  nlParam.minValue = 1.0;
  nlParam.maxValue = 100.0;
  nlParam.category = ParameterCategory::ADVANCED;
  nlParam.unit = "";
  nlParam.stepSize = 1.0;
  nlParam.required = false;
  ionBeamProcess.parameters.push_back(nlParam);

  ParameterMetadata inflectAngleParam;
  inflectAngleParam.name = "InflectAngle";
  inflectAngleParam.displayName = "Inflection Angle";
  inflectAngleParam.documentation =
      "Angle for energy reflection inflection point (degrees)";
  inflectAngleParam.type = ParameterType::DOUBLE;
  inflectAngleParam.defaultValue = 89.0;
  inflectAngleParam.minValue = 45.0;
  inflectAngleParam.maxValue = 90.0;
  inflectAngleParam.category = ParameterCategory::ADVANCED;
  inflectAngleParam.unit = "°";
  inflectAngleParam.stepSize = 1.0;
  inflectAngleParam.required = false;
  ionBeamProcess.parameters.push_back(inflectAngleParam);

  ParameterMetadata minAngleParam;
  minAngleParam.name = "MinAngle";
  minAngleParam.displayName = "Minimum Reflection Angle";
  minAngleParam.documentation =
      "Minimum angle for reflected particles (degrees)";
  minAngleParam.type = ParameterType::DOUBLE;
  minAngleParam.defaultValue = 85.0;
  minAngleParam.minValue = 45.0;
  minAngleParam.maxValue = 90.0;
  minAngleParam.category = ParameterCategory::ADVANCED;
  minAngleParam.unit = "°";
  minAngleParam.stepSize = 1.0;
  minAngleParam.required = false;
  ionBeamProcess.parameters.push_back(minAngleParam);

  ParameterMetadata redepositionRateParam;
  redepositionRateParam.name = "RedepositionRate";
  redepositionRateParam.displayName = "Redeposition Rate";
  redepositionRateParam.documentation =
      "Rate of material redeposition (0 = disabled)";
  redepositionRateParam.type = ParameterType::DOUBLE;
  redepositionRateParam.defaultValue = 0.0;
  redepositionRateParam.minValue = 0.0;
  redepositionRateParam.maxValue = 1.0;
  redepositionRateParam.category = ParameterCategory::ADVANCED;
  redepositionRateParam.unit = "";
  redepositionRateParam.stepSize = 0.01;
  redepositionRateParam.required = false;
  ionBeamProcess.parameters.push_back(redepositionRateParam);

  ParameterMetadata redepositionThresholdParam;
  redepositionThresholdParam.name = "RedepositionThreshold";
  redepositionThresholdParam.displayName = "Redeposition Threshold";
  redepositionThresholdParam.documentation =
      "Threshold for material redeposition";
  redepositionThresholdParam.type = ParameterType::DOUBLE;
  redepositionThresholdParam.defaultValue = 0.1;
  redepositionThresholdParam.minValue = 0.0;
  redepositionThresholdParam.maxValue = 1.0;
  redepositionThresholdParam.category = ParameterCategory::ADVANCED;
  redepositionThresholdParam.unit = "";
  redepositionThresholdParam.stepSize = 0.01;
  redepositionThresholdParam.required = false;
  ionBeamProcess.parameters.push_back(redepositionThresholdParam);

  ParameterMetadata yieldFunctionParam;
  yieldFunctionParam.name = "YieldFunction";
  yieldFunctionParam.displayName = "Yield Function Type";
  yieldFunctionParam.documentation = "Angular dependence of sputtering yield";
  yieldFunctionParam.type = ParameterType::ENUM;
  yieldFunctionParam.defaultValue = 0;
  yieldFunctionParam.enumOptions = {"Constant", "Cosine", "Cosine Power",
                                    "Custom", "Cos4"};
  yieldFunctionParam.category = ParameterCategory::ADVANCED;
  yieldFunctionParam.required = false;
  ionBeamProcess.parameters.push_back(yieldFunctionParam);

  ParameterMetadata yieldPowerParam;
  yieldPowerParam.name = "YieldPower";
  yieldPowerParam.displayName = "Yield Function Power";
  yieldPowerParam.documentation = "Power for cosine yield function";
  yieldPowerParam.type = ParameterType::DOUBLE;
  yieldPowerParam.defaultValue = 1.0;
  yieldPowerParam.minValue = 0.1;
  yieldPowerParam.maxValue = 5.0;
  yieldPowerParam.category = ParameterCategory::ADVANCED;
  yieldPowerParam.unit = "";
  yieldPowerParam.stepSize = 0.1;
  yieldPowerParam.required = false;
  ionBeamProcess.parameters.push_back(yieldPowerParam);

  ParameterMetadata thetaRMinParam;
  thetaRMinParam.name = "ThetaRMin";
  thetaRMinParam.displayName = "Min Sticking Reflection Angle";
  thetaRMinParam.documentation = "Minimum sticking reflection angle (degrees)";
  thetaRMinParam.type = ParameterType::DOUBLE;
  thetaRMinParam.defaultValue = 70.0;
  thetaRMinParam.minValue = 0.0;
  thetaRMinParam.maxValue = 90.0;
  thetaRMinParam.category = ParameterCategory::ADVANCED;
  thetaRMinParam.unit = "°";
  thetaRMinParam.stepSize = 1.0;
  thetaRMinParam.required = false;
  ionBeamProcess.parameters.push_back(thetaRMinParam);

  ParameterMetadata thetaRMaxParam;
  thetaRMaxParam.name = "ThetaRMax";
  thetaRMaxParam.displayName = "Max Sticking Reflection Angle";
  thetaRMaxParam.documentation = "Maximum sticking reflection angle (degrees)";
  thetaRMaxParam.type = ParameterType::DOUBLE;
  thetaRMaxParam.defaultValue = 90.0;
  thetaRMaxParam.minValue = 0.0;
  thetaRMaxParam.maxValue = 90.0;
  thetaRMaxParam.category = ParameterCategory::ADVANCED;
  thetaRMaxParam.unit = "°";
  thetaRMaxParam.stepSize = 1.0;
  thetaRMaxParam.required = false;
  ionBeamProcess.parameters.push_back(thetaRMaxParam);

  ParameterMetadata rotatingWaferParam;
  rotatingWaferParam.name = "RotatingWafer";
  rotatingWaferParam.displayName = "Rotating Wafer";
  rotatingWaferParam.documentation = "Enable rotating wafer mode";
  rotatingWaferParam.type = ParameterType::BOOLEAN;
  rotatingWaferParam.defaultValue = 0;
  rotatingWaferParam.category = ParameterCategory::ADVANCED;
  rotatingWaferParam.required = false;
  ionBeamProcess.parameters.push_back(rotatingWaferParam);

  ParameterMetadata cos4A1Param;
  cos4A1Param.name = "Cos4_a1";
  cos4A1Param.displayName = "Cos4 Yield a1";
  cos4A1Param.documentation = "Cos4 yield function parameter a1";
  cos4A1Param.type = ParameterType::DOUBLE;
  cos4A1Param.defaultValue = 0.0;
  cos4A1Param.minValue = -100.0;
  cos4A1Param.maxValue = 100.0;
  cos4A1Param.category = ParameterCategory::ADVANCED;
  cos4A1Param.stepSize = 0.1;
  cos4A1Param.required = false;
  ionBeamProcess.parameters.push_back(cos4A1Param);

  ParameterMetadata cos4A2Param;
  cos4A2Param.name = "Cos4_a2";
  cos4A2Param.displayName = "Cos4 Yield a2";
  cos4A2Param.documentation = "Cos4 yield function parameter a2";
  cos4A2Param.type = ParameterType::DOUBLE;
  cos4A2Param.defaultValue = 0.0;
  cos4A2Param.minValue = -100.0;
  cos4A2Param.maxValue = 100.0;
  cos4A2Param.category = ParameterCategory::ADVANCED;
  cos4A2Param.stepSize = 0.1;
  cos4A2Param.required = false;
  ionBeamProcess.parameters.push_back(cos4A2Param);

  ParameterMetadata cos4A3Param;
  cos4A3Param.name = "Cos4_a3";
  cos4A3Param.displayName = "Cos4 Yield a3";
  cos4A3Param.documentation = "Cos4 yield function parameter a3";
  cos4A3Param.type = ParameterType::DOUBLE;
  cos4A3Param.defaultValue = 0.0;
  cos4A3Param.minValue = -100.0;
  cos4A3Param.maxValue = 100.0;
  cos4A3Param.category = ParameterCategory::ADVANCED;
  cos4A3Param.stepSize = 0.1;
  cos4A3Param.required = false;
  ionBeamProcess.parameters.push_back(cos4A3Param);

  ParameterMetadata cos4A4Param;
  cos4A4Param.name = "Cos4_a4";
  cos4A4Param.displayName = "Cos4 Yield a4";
  cos4A4Param.documentation = "Cos4 yield function parameter a4";
  cos4A4Param.type = ParameterType::DOUBLE;
  cos4A4Param.defaultValue = 0.0;
  cos4A4Param.minValue = -100.0;
  cos4A4Param.maxValue = 100.0;
  cos4A4Param.category = ParameterCategory::ADVANCED;
  cos4A4Param.stepSize = 0.1;
  cos4A4Param.required = false;
  ionBeamProcess.parameters.push_back(cos4A4Param);

  auto factory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                    vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 10.0);
    double planeWaferRate =
        registry.getParameter<double>(params, "PlaneWaferRate", 1.0);
    double meanEnergy =
        registry.getParameter<double>(params, "MeanEnergy", 250.0);
    double sigmaEnergy =
        registry.getParameter<double>(params, "SigmaEnergy", 10.0);
    double thresholdEnergy =
        registry.getParameter<double>(params, "ThresholdEnergy", 20.0);
    double tiltAngle = registry.getParameter<double>(params, "TiltAngle", 0.0);
    double ionExponent =
        registry.getParameter<double>(params, "IonExponent", 100.0);
    double n_l = registry.getParameter<double>(params, "NL", 10.0);
    double inflectAngle =
        registry.getParameter<double>(params, "InflectAngle", 89.0);
    double minAngle = registry.getParameter<double>(params, "MinAngle", 85.0);
    double redepositionRate =
        registry.getParameter<double>(params, "RedepositionRate", 0.0);
    double redepositionThreshold =
        registry.getParameter<double>(params, "RedepositionThreshold", 0.1);
    int yieldFunctionType =
        registry.getParameter<int>(params, "YieldFunction", 0);
    double yieldPower =
        registry.getParameter<double>(params, "YieldPower", 1.0);
    double thetaRMin = registry.getParameter<double>(params, "ThetaRMin", 70.0);
    double thetaRMax = registry.getParameter<double>(params, "ThetaRMax", 90.0);
    bool rotatingWafer =
        registry.getParameter<int>(params, "RotatingWafer", 0) != 0;
    double cos4_a1 = registry.getParameter<double>(params, "Cos4_a1", 0.0);
    double cos4_a2 = registry.getParameter<double>(params, "Cos4_a2", 0.0);
    double cos4_a3 = registry.getParameter<double>(params, "Cos4_a3", 0.0);
    double cos4_a4 = registry.getParameter<double>(params, "Cos4_a4", 0.0);

    std::vector<viennaps::Material> maskMaterials;
    if (params.find("MaskMaterials") != params.end()) {
      auto matList = registry.getParameter<MaterialListValue>(
          params, "MaskMaterials", MaterialListValue{{1}});

      for (int matId : matList.materialIds) {
        auto material = static_cast<viennaps::Material>(matId);
        if (material != viennaps::Material::Undefined) {
          maskMaterials.push_back(material);
        }
      }
    } else {
      maskMaterials.push_back(viennaps::Material::Mask);
    }

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;

          viennaps::IBEParameters<NumericType> ibeParams;
          ibeParams.planeWaferRate = planeWaferRate;
          ibeParams.meanEnergy = meanEnergy;
          ibeParams.sigmaEnergy = sigmaEnergy;
          ibeParams.thresholdEnergy = thresholdEnergy;
          ibeParams.tiltAngle = tiltAngle;
          ibeParams.exponent = ionExponent;
          ibeParams.n_l = n_l;
          ibeParams.inflectAngle = inflectAngle;
          ibeParams.minAngle = minAngle;
          ibeParams.redepositionRate = redepositionRate;
          ibeParams.redepositionThreshold = redepositionThreshold;
          ibeParams.thetaRMin = thetaRMin;
          ibeParams.thetaRMax = thetaRMax;
          ibeParams.rotatingWafer = rotatingWafer;

          switch (yieldFunctionType) {
          case 0:
            ibeParams.yieldFunction = [](NumericType theta) { return 1.0; };
            break;
          case 1:
            ibeParams.yieldFunction = [](NumericType theta) {
              return std::cos(theta);
            };
            break;
          case 2:
            ibeParams.yieldFunction = [yieldPower](NumericType theta) {
              return std::pow(std::cos(theta), yieldPower);
            };
            break;
          case 3:
            ibeParams.yieldFunction = [](NumericType theta) { return 1.0; };
            break;
          case 4: // Cos4
            ibeParams.cos4Yield.a1 = cos4_a1;
            ibeParams.cos4Yield.a2 = cos4_a2;
            ibeParams.cos4Yield.a3 = cos4_a3;
            ibeParams.cos4Yield.a4 = cos4_a4;
            ibeParams.cos4Yield.isDefined = true;
            break;
          default:
            ibeParams.yieldFunction = [](NumericType theta) { return 1.0; };
            break;
          }

          auto model = viennaps::SmartPointer<
              viennaps::IonBeamEtching<NumericType, Dim>>::New(ibeParams,
                                                               maskMaterials);

          VPSLOG_DEBUG(nullptr, "Plane wafer rate: ", planeWaferRate, " nm/s");
          VPSLOG_DEBUG(nullptr, "Mean ion energy: ", meanEnergy, " eV");
          VPSLOG_DEBUG(nullptr, "Energy sigma: ", sigmaEnergy, " eV");
          VPSLOG_DEBUG(nullptr, "Threshold energy: ", thresholdEnergy, " eV");
          VPSLOG_DEBUG(nullptr, "Beam tilt angle: ", tiltAngle, " degrees");
          VPSLOG_DEBUG(nullptr, "Mask materials: ", maskMaterials.size());
          if (redepositionRate > 0) {
            VPSLOG_DEBUG(nullptr,
                         "Redeposition enabled - Rate: ", redepositionRate,
                         ", Threshold: ", redepositionThreshold);
          }

          viennaps::Process<NumericType, Dim> process;
          process.setDomain(psDomain);
          process.setProcessModel(model);
          {
            viennaps::RayTracingParameters rayParams;
            rayParams.raysPerPoint = static_cast<unsigned>(
                registry.getParameter<int>(params, "NumRaysPerPoint", 1000));
            process.setParameters(rayParams);
          }

#ifndef MULTI_STEP
          process.setProcessDuration(processTime);
          process.apply();
#else
                const int numSteps = 20;
                const double stepTime = processTime / numSteps;

                process.setProcessDuration(stepTime);
                for (int step = 0; step < numSteps; ++step) {
                    process.apply();
                }
#endif

          ViennaPSModels::convertToVTK<Dim>(psDomain, output, params);
        });
  };

  ionBeamProcess.parameters.push_back(makeNumRaysPerPointParam());

  vtkViennaPSModelRegistry::getInstance().registerProcessModel(
      "IonBeamEtching", ionBeamProcess, factory);
}

void registerHBrO2EtchingProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata hbro2Process;
  hbro2Process.className = "HBrO2Etching";
  hbro2Process.displayName = "HBr/O2 Plasma Etching";
  hbro2Process.description =
      "HBr/O2 plasma etching model for Si with passivation";
  hbro2Process.type = ModelType::SIMULATION;

  ParameterMetadata ionFluxParam;
  ionFluxParam.name = "IonFlux";
  ionFluxParam.displayName = "Ion Flux";
  ionFluxParam.documentation = "Ion flux (1e15 /cm²/s)";
  ionFluxParam.type = ParameterType::DOUBLE;
  ionFluxParam.defaultValue = 12.0;
  ionFluxParam.minValue = 0.0;
  ionFluxParam.maxValue = 100.0;
  ionFluxParam.category = ParameterCategory::BASIC;
  ionFluxParam.unit = "";
  ionFluxParam.stepSize = 1.0;
  ionFluxParam.required = true;
  hbro2Process.parameters.push_back(ionFluxParam);

  ParameterMetadata etchantFluxParam;
  etchantFluxParam.name = "EtchantFlux";
  etchantFluxParam.displayName = "Etchant (HBr) Flux";
  etchantFluxParam.documentation = "HBr etchant flux (1e15 /cm²/s)";
  etchantFluxParam.type = ParameterType::DOUBLE;
  etchantFluxParam.defaultValue = 1800.0;
  etchantFluxParam.minValue = 0.0;
  etchantFluxParam.maxValue = 10000.0;
  etchantFluxParam.category = ParameterCategory::BASIC;
  etchantFluxParam.unit = "";
  etchantFluxParam.stepSize = 100.0;
  etchantFluxParam.required = true;
  hbro2Process.parameters.push_back(etchantFluxParam);

  ParameterMetadata oxygenFluxParam;
  oxygenFluxParam.name = "OxygenFlux";
  oxygenFluxParam.displayName = "Oxygen Flux";
  oxygenFluxParam.documentation = "Oxygen passivation flux (1e15 /cm²/s)";
  oxygenFluxParam.type = ParameterType::DOUBLE;
  oxygenFluxParam.defaultValue = 100.0;
  oxygenFluxParam.minValue = 0.0;
  oxygenFluxParam.maxValue = 1000.0;
  oxygenFluxParam.category = ParameterCategory::BASIC;
  oxygenFluxParam.unit = "";
  oxygenFluxParam.stepSize = 10.0;
  oxygenFluxParam.required = true;
  hbro2Process.parameters.push_back(oxygenFluxParam);

  ParameterMetadata meanEnergyParam;
  meanEnergyParam.name = "MeanEnergy";
  meanEnergyParam.displayName = "Mean Ion Energy";
  meanEnergyParam.documentation = "Mean energy of ions";
  meanEnergyParam.type = ParameterType::DOUBLE;
  meanEnergyParam.defaultValue = 100.0;
  meanEnergyParam.minValue = 10.0;
  meanEnergyParam.maxValue = 500.0;
  meanEnergyParam.category = ParameterCategory::BASIC;
  meanEnergyParam.unit = "";
  meanEnergyParam.stepSize = 10.0;
  meanEnergyParam.required = true;
  hbro2Process.parameters.push_back(meanEnergyParam);

  ParameterMetadata sigmaEnergyParam;
  sigmaEnergyParam.name = "SigmaEnergy";
  sigmaEnergyParam.displayName = "Ion Energy Sigma";
  sigmaEnergyParam.documentation =
      "Standard deviation of ion energy distribution";
  sigmaEnergyParam.type = ParameterType::DOUBLE;
  sigmaEnergyParam.defaultValue = 10.0;
  sigmaEnergyParam.minValue = 0.1;
  sigmaEnergyParam.maxValue = 100.0;
  sigmaEnergyParam.category = ParameterCategory::BASIC;
  sigmaEnergyParam.unit = "";
  sigmaEnergyParam.stepSize = 1.0;
  sigmaEnergyParam.required = true;
  hbro2Process.parameters.push_back(sigmaEnergyParam);

  ParameterMetadata ionExponentParam;
  ionExponentParam.name = "IonExponent";
  ionExponentParam.displayName = "Ion Angular Distribution";
  ionExponentParam.documentation = "Exponent for ion angular distribution";
  ionExponentParam.type = ParameterType::DOUBLE;
  ionExponentParam.defaultValue = 300.0;
  ionExponentParam.minValue = 1.0;
  ionExponentParam.maxValue = 1000.0;
  ionExponentParam.category = ParameterCategory::ADVANCED;
  ionExponentParam.unit = "";
  ionExponentParam.stepSize = 50.0;
  ionExponentParam.required = false;
  hbro2Process.parameters.push_back(ionExponentParam);

  ParameterMetadata oxySputterParam;
  oxySputterParam.name = "OxygenSputterYield";
  oxySputterParam.displayName = "Oxygen Sputter Yield";
  oxySputterParam.documentation = "Ion-enhanced oxygen sputtering yield";
  oxySputterParam.type = ParameterType::DOUBLE;
  oxySputterParam.defaultValue = 2.0;
  oxySputterParam.minValue = 0.0;
  oxySputterParam.maxValue = 10.0;
  oxySputterParam.category = ParameterCategory::ADVANCED;
  oxySputterParam.unit = "";
  oxySputterParam.stepSize = 0.5;
  oxySputterParam.required = false;
  hbro2Process.parameters.push_back(oxySputterParam);

  ParameterMetadata etchStopParam;
  etchStopParam.name = "EtchStopDepth";
  etchStopParam.displayName = "Etch Stop Depth";
  etchStopParam.documentation =
      "Z-coordinate to stop etching. Leave at minimum for no stop.";
  etchStopParam.type = ParameterType::DOUBLE;
  etchStopParam.defaultValue = std::numeric_limits<double>::lowest();
  etchStopParam.minValue = std::numeric_limits<double>::lowest();
  etchStopParam.maxValue = 0.0;
  etchStopParam.category = ParameterCategory::ADVANCED;
  etchStopParam.unit = "";
  etchStopParam.stepSize = 10.0;
  etchStopParam.required = false;
  hbro2Process.parameters.push_back(etchStopParam);

  ParameterMetadata betaEParam;
  betaEParam.name = "BetaE";
  betaEParam.displayName = "Etchant Sticking Coefficient";
  betaEParam.documentation = "Sticking probability of etchant species";
  betaEParam.type = ParameterType::DOUBLE;
  betaEParam.defaultValue = 0.1;
  betaEParam.minValue = 0.0;
  betaEParam.maxValue = 1.0;
  betaEParam.category = ParameterCategory::ADVANCED;
  betaEParam.unit = "";
  betaEParam.stepSize = 0.05;
  betaEParam.required = false;
  hbro2Process.parameters.push_back(betaEParam);

  ParameterMetadata betaPParam;
  betaPParam.name = "BetaP";
  betaPParam.displayName = "Passivation Sticking Coefficient";
  betaPParam.documentation = "Sticking probability of passivation species";
  betaPParam.type = ParameterType::DOUBLE;
  betaPParam.defaultValue = 1.0;
  betaPParam.minValue = 0.0;
  betaPParam.maxValue = 1.0;
  betaPParam.category = ParameterCategory::ADVANCED;
  betaPParam.unit = "";
  betaPParam.stepSize = 0.05;
  betaPParam.required = false;
  hbro2Process.parameters.push_back(betaPParam);

  ParameterMetadata kSigmaParam;
  kSigmaParam.name = "KSigma";
  kSigmaParam.displayName = "Chemical Etching Rate";
  kSigmaParam.documentation = "Chemical etching rate constant (1e15 cm⁻²s⁻¹)";
  kSigmaParam.type = ParameterType::DOUBLE;
  kSigmaParam.defaultValue = 300.0;
  kSigmaParam.minValue = 0.0;
  kSigmaParam.maxValue = 1000.0;
  kSigmaParam.category = ParameterCategory::ADVANCED;
  kSigmaParam.unit = "";
  kSigmaParam.stepSize = 10.0;
  kSigmaParam.required = false;
  hbro2Process.parameters.push_back(kSigmaParam);

  ParameterMetadata lengthUnitParam;
  lengthUnitParam.name = "LengthUnit";
  lengthUnitParam.displayName = "Length Unit";
  lengthUnitParam.documentation = "Unit for length measurements";
  lengthUnitParam.type = ParameterType::ENUM;
  lengthUnitParam.defaultValue = 0;
  lengthUnitParam.category = ParameterCategory::BASIC;
  lengthUnitParam.required = true;
  lengthUnitParam.enumOptions = {"Nanometer",  "Micrometer", "Millimeter",
                                 "Centimeter", "Meter",      "Angstrom"};
  hbro2Process.parameters.push_back(lengthUnitParam);

  ParameterMetadata timeUnitParam;
  timeUnitParam.name = "TimeUnit";
  timeUnitParam.displayName = "Time Unit";
  timeUnitParam.documentation = "Unit for time measurements";
  timeUnitParam.type = ParameterType::ENUM;
  timeUnitParam.defaultValue = 0;
  timeUnitParam.enumOptions = {"Second", "Minute", "Millisecond"};
  timeUnitParam.category = ParameterCategory::BASIC;
  timeUnitParam.required = true;
  hbro2Process.parameters.push_back(timeUnitParam);

  auto factory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                    vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    int lengthUnit = registry.getParameter<int>(params, "LengthUnit", 0);
    int timeUnit = registry.getParameter<int>(params, "TimeUnit", 0);

    switch (lengthUnit) {
    case 0:
      viennaps::units::Length::setUnit(viennaps::units::Length::NANOMETER);
      break;
    case 1:
      viennaps::units::Length::setUnit(viennaps::units::Length::MICROMETER);
      break;
    case 2:
      viennaps::units::Length::setUnit(viennaps::units::Length::MILLIMETER);
      break;
    case 3:
      viennaps::units::Length::setUnit(viennaps::units::Length::CENTIMETER);
      break;
    case 4:
      viennaps::units::Length::setUnit(viennaps::units::Length::METER);
      break;
    case 5:
      viennaps::units::Length::setUnit(viennaps::units::Length::ANGSTROM);
      break;
    default:
      viennaps::units::Length::setUnit(viennaps::units::Length::NANOMETER);
      break;
    }

    switch (timeUnit) {
    case 0:
      viennaps::units::Time::setUnit(viennaps::units::Time::SECOND);
      break;
    case 1:
      viennaps::units::Time::setUnit(viennaps::units::Time::MINUTE);
      break;
    case 2:
      viennaps::units::Time::setUnit(viennaps::units::Time::MILLISECOND);
      break;
    default:
      viennaps::units::Time::setUnit(viennaps::units::Time::SECOND);
      break;
    }

    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 10.0);
    double ionFlux = registry.getParameter<double>(params, "IonFlux", 12.0);
    double etchantFlux =
        registry.getParameter<double>(params, "EtchantFlux", 1800.0);
    double oxygenFlux =
        registry.getParameter<double>(params, "OxygenFlux", 100.0);
    double meanEnergy =
        registry.getParameter<double>(params, "MeanEnergy", 100.0);
    double sigmaEnergy =
        registry.getParameter<double>(params, "SigmaEnergy", 10.0);
    double ionExponent =
        registry.getParameter<double>(params, "IonExponent", 500.0);
    double oxySputterYield =
        registry.getParameter<double>(params, "OxygenSputterYield", 3.0);
    double etchStopDepth = registry.getParameter<double>(
        params, "EtchStopDepth", std::numeric_limits<NumericType>::lowest());
    double betaE = registry.getParameter<double>(params, "BetaE", 0.1);
    double betaP = registry.getParameter<double>(params, "BetaP", 1.0);
    double kSigma = registry.getParameter<double>(params, "KSigma", 300.0);

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;

          auto hbrParams =
              viennaps::HBrO2Etching<NumericType, Dim>::defaultParameters();
          hbrParams.ionFlux = ionFlux;
          hbrParams.etchantFlux = etchantFlux;
          hbrParams.passivationFlux = oxygenFlux;
          hbrParams.Ions.meanEnergy = meanEnergy;
          hbrParams.Ions.sigmaEnergy = sigmaEnergy;
          hbrParams.Ions.exponent = ionExponent;
          hbrParams.Passivation.A_ie = oxySputterYield;
          hbrParams.etchStopDepth = etchStopDepth;
          hbrParams.Substrate.k_sigma = kSigma;

          hbrParams.beta_E.set(viennaps::Material::Si, betaE);
          hbrParams.beta_E.set(viennaps::Material::Mask, betaE);
          hbrParams.beta_P.set(viennaps::Material::Si, betaP);
          hbrParams.beta_P.set(viennaps::Material::Mask, betaP);

          auto model = viennaps::SmartPointer<
              viennaps::HBrO2Etching<NumericType, Dim>>::New(hbrParams);

          VPSLOG_DEBUG(nullptr, "Ion flux: ", ionFlux, " (1e15 /cm²/s)");
          VPSLOG_DEBUG(nullptr, "Etchant flux: ", etchantFlux,
                       " (1e15 /cm²/s)");
          VPSLOG_DEBUG(nullptr, "Oxygen flux: ", oxygenFlux, " (1e15 /cm²/s)");
          VPSLOG_DEBUG(nullptr, "Mean ion energy: ", meanEnergy, " eV");
          VPSLOG_DEBUG(nullptr, "Ion energy sigma: ", sigmaEnergy, " eV");
          if (etchStopDepth > std::numeric_limits<NumericType>::lowest()) {
            VPSLOG_DEBUG(nullptr, "Etch stop depth: ", etchStopDepth, " nm");
          }

          viennaps::Process<NumericType, Dim> process;
          process.setDomain(psDomain);
          process.setProcessModel(model);
          {
            viennaps::RayTracingParameters rayParams;
            rayParams.raysPerPoint = static_cast<unsigned>(
                registry.getParameter<int>(params, "NumRaysPerPoint", 1000));
            process.setParameters(rayParams);
          }

#ifndef MULTI_STEP
          process.setProcessDuration(processTime);
          process.apply();
#else
                const int numSteps = 20;
                const double stepTime = processTime / numSteps;

                process.setProcessDuration(stepTime);
                for (int step = 0; step < numSteps; ++step) {
                    process.apply();
                }
#endif

          ViennaPSModels::convertToVTK<Dim>(psDomain, output, params);
        });
  };

  hbro2Process.parameters.push_back(makeNumRaysPerPointParam());

  vtkViennaPSModelRegistry::getInstance().registerProcessModel(
      "HBrO2Etching", hbro2Process, factory);
}

void registerFluorocarbonEtchingProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata fluorocarbonProcess;
  fluorocarbonProcess.className = "FluorocarbonEtching";
  fluorocarbonProcess.displayName = "Fluorocarbon Plasma Etching";
  fluorocarbonProcess.description = "Fluorocarbon plasma etching model for "
                                    "SiO2, Si, Si3N4 with polymer deposition";
  fluorocarbonProcess.type = ModelType::SIMULATION;

  ParameterMetadata ionFluxParam;
  ionFluxParam.name = "IonFlux";
  ionFluxParam.displayName = "Ion Flux";
  ionFluxParam.documentation = "Ion flux (1e15 /cm²/s)";
  ionFluxParam.type = ParameterType::DOUBLE;
  ionFluxParam.defaultValue = 56.0;
  ionFluxParam.minValue = 0.0;
  ionFluxParam.maxValue = 500.0;
  ionFluxParam.category = ParameterCategory::BASIC;
  ionFluxParam.unit = "";
  ionFluxParam.stepSize = 5.0;
  ionFluxParam.required = true;
  fluorocarbonProcess.parameters.push_back(ionFluxParam);

  ParameterMetadata etchantFluxParam;
  etchantFluxParam.name = "EtchantFlux";
  etchantFluxParam.displayName = "Etchant Flux";
  etchantFluxParam.documentation = "Etchant flux (1e15 /cm²/s)";
  etchantFluxParam.type = ParameterType::DOUBLE;
  etchantFluxParam.defaultValue = 500.0;
  etchantFluxParam.minValue = 0.0;
  etchantFluxParam.maxValue = 5000.0;
  etchantFluxParam.category = ParameterCategory::BASIC;
  etchantFluxParam.unit = "";
  etchantFluxParam.stepSize = 50.0;
  etchantFluxParam.required = true;
  fluorocarbonProcess.parameters.push_back(etchantFluxParam);

  ParameterMetadata polymerFluxParam;
  polymerFluxParam.name = "PolymerFlux";
  polymerFluxParam.displayName = "Polymer Flux";
  polymerFluxParam.documentation = "Polymer flux (1e15 /cm²/s)";
  polymerFluxParam.type = ParameterType::DOUBLE;
  polymerFluxParam.defaultValue = 100.0;
  polymerFluxParam.minValue = 0.0;
  polymerFluxParam.maxValue = 1000.0;
  polymerFluxParam.category = ParameterCategory::BASIC;
  polymerFluxParam.unit = "";
  polymerFluxParam.stepSize = 10.0;
  polymerFluxParam.required = true;
  fluorocarbonProcess.parameters.push_back(polymerFluxParam);

  ParameterMetadata meanEnergyParam;
  meanEnergyParam.name = "MeanEnergy";
  meanEnergyParam.displayName = "Mean Ion Energy";
  meanEnergyParam.documentation = "Mean energy of ions";
  meanEnergyParam.type = ParameterType::DOUBLE;
  meanEnergyParam.defaultValue = 100.0;
  meanEnergyParam.minValue = 10.0;
  meanEnergyParam.maxValue = 500.0;
  meanEnergyParam.category = ParameterCategory::BASIC;
  meanEnergyParam.unit = "";
  meanEnergyParam.stepSize = 10.0;
  meanEnergyParam.required = true;
  fluorocarbonProcess.parameters.push_back(meanEnergyParam);

  ParameterMetadata sigmaEnergyParam;
  sigmaEnergyParam.name = "SigmaEnergy";
  sigmaEnergyParam.displayName = "Ion Energy Sigma";
  sigmaEnergyParam.documentation =
      "Standard deviation of ion energy distribution";
  sigmaEnergyParam.type = ParameterType::DOUBLE;
  sigmaEnergyParam.defaultValue = 10.0;
  sigmaEnergyParam.minValue = 0.1;
  sigmaEnergyParam.maxValue = 100.0;
  sigmaEnergyParam.category = ParameterCategory::BASIC;
  sigmaEnergyParam.unit = "";
  sigmaEnergyParam.stepSize = 1.0;
  sigmaEnergyParam.required = true;
  fluorocarbonProcess.parameters.push_back(sigmaEnergyParam);

  ParameterMetadata temperatureParam;
  temperatureParam.name = "Temperature";
  temperatureParam.displayName = "Process Temperature";
  temperatureParam.documentation = "Process temperature";
  temperatureParam.type = ParameterType::DOUBLE;
  temperatureParam.defaultValue = 300.0;
  temperatureParam.minValue = 100.0;
  temperatureParam.maxValue = 600.0;
  temperatureParam.category = ParameterCategory::BASIC;
  temperatureParam.unit = "";
  temperatureParam.stepSize = 10.0;
  temperatureParam.required = true;
  fluorocarbonProcess.parameters.push_back(temperatureParam);

  ParameterMetadata ionExponentParam;
  ionExponentParam.name = "IonExponent";
  ionExponentParam.displayName = "Ion Angular Distribution";
  ionExponentParam.documentation = "Exponent for ion angular distribution";
  ionExponentParam.type = ParameterType::DOUBLE;
  ionExponentParam.defaultValue = 500.0;
  ionExponentParam.minValue = 1.0;
  ionExponentParam.maxValue = 1000.0;
  ionExponentParam.category = ParameterCategory::ADVANCED;
  ionExponentParam.unit = "";
  ionExponentParam.stepSize = 50.0;
  ionExponentParam.required = false;
  fluorocarbonProcess.parameters.push_back(ionExponentParam);

  ParameterMetadata deltaPParam;
  deltaPParam.name = "DeltaP";
  deltaPParam.displayName = "Delta P";
  deltaPParam.documentation = "Polymer coverage parameter";
  deltaPParam.type = ParameterType::DOUBLE;
  deltaPParam.defaultValue = 1.0;
  deltaPParam.minValue = 0.0;
  deltaPParam.maxValue = 10.0;
  deltaPParam.category = ParameterCategory::ADVANCED;
  deltaPParam.unit = "";
  deltaPParam.stepSize = 0.1;
  deltaPParam.required = false;
  fluorocarbonProcess.parameters.push_back(deltaPParam);

  ParameterMetadata etchStopParam;
  etchStopParam.name = "EtchStopDepth";
  etchStopParam.displayName = "Etch Stop Depth";
  etchStopParam.documentation =
      "Z-coordinate to stop etching. Leave at minimum for no stop.";
  etchStopParam.type = ParameterType::DOUBLE;
  etchStopParam.defaultValue = std::numeric_limits<double>::lowest();
  etchStopParam.minValue = std::numeric_limits<double>::lowest();
  etchStopParam.maxValue = 0.0;
  etchStopParam.category = ParameterCategory::ADVANCED;
  etchStopParam.unit = "";
  etchStopParam.stepSize = 10.0;
  etchStopParam.required = false;
  fluorocarbonProcess.parameters.push_back(etchStopParam);

  ParameterMetadata kieParam;
  kieParam.name = "K_IE";
  kieParam.displayName = "Ion-enhanced etching coefficient";
  kieParam.documentation = "Coefficient for ion-enhanced etching";
  kieParam.type = ParameterType::DOUBLE;
  kieParam.defaultValue = 2.0;
  kieParam.minValue = 0.1;
  kieParam.maxValue = 10.0;
  kieParam.category = ParameterCategory::ADVANCED;
  kieParam.unit = "";
  kieParam.stepSize = 0.1;
  kieParam.required = false;
  fluorocarbonProcess.parameters.push_back(kieParam);

  ParameterMetadata kevParam;
  kevParam.name = "K_EV";
  kevParam.displayName = "Evaporation coefficient";
  kevParam.documentation = "Coefficient for evaporation";
  kevParam.type = ParameterType::DOUBLE;
  kevParam.defaultValue = 2.0;
  kevParam.minValue = 0.1;
  kevParam.maxValue = 10.0;
  kevParam.category = ParameterCategory::ADVANCED;
  kevParam.unit = "";
  kevParam.stepSize = 0.1;
  kevParam.required = false;
  fluorocarbonProcess.parameters.push_back(kevParam);

  ParameterMetadata betaPParam;
  betaPParam.name = "BetaP";
  betaPParam.displayName = "Polymer sticking";
  betaPParam.documentation = "Sticking probability of polymer";
  betaPParam.type = ParameterType::DOUBLE;
  betaPParam.defaultValue = 0.26;
  betaPParam.minValue = 0.0;
  betaPParam.maxValue = 1.0;
  betaPParam.category = ParameterCategory::ADVANCED;
  betaPParam.unit = "";
  betaPParam.stepSize = 0.05;
  betaPParam.required = false;
  fluorocarbonProcess.parameters.push_back(betaPParam);

  ParameterMetadata betaEParam;
  betaEParam.name = "BetaE";
  betaEParam.displayName = "Etchant sticking";
  betaEParam.documentation = "Sticking probability of etchant";
  betaEParam.type = ParameterType::DOUBLE;
  betaEParam.defaultValue = 0.9;
  betaEParam.minValue = 0.0;
  betaEParam.maxValue = 1.0;
  betaEParam.category = ParameterCategory::ADVANCED;
  betaEParam.unit = "";
  betaEParam.stepSize = 0.05;
  betaEParam.required = false;
  fluorocarbonProcess.parameters.push_back(betaEParam);

  ParameterMetadata lengthUnitParam;
  lengthUnitParam.name = "LengthUnit";
  lengthUnitParam.displayName = "Length Unit";
  lengthUnitParam.documentation = "Unit for length measurements";
  lengthUnitParam.type = ParameterType::ENUM;
  lengthUnitParam.defaultValue = 0;
  lengthUnitParam.category = ParameterCategory::BASIC;
  lengthUnitParam.required = true;
  lengthUnitParam.enumOptions = {"Nanometer",  "Micrometer", "Millimeter",
                                 "Centimeter", "Meter",      "Angstrom"};
  fluorocarbonProcess.parameters.push_back(lengthUnitParam);

  ParameterMetadata timeUnitParam;
  timeUnitParam.name = "TimeUnit";
  timeUnitParam.displayName = "Time Unit";
  timeUnitParam.documentation = "Unit for time measurements";
  timeUnitParam.type = ParameterType::ENUM;
  timeUnitParam.defaultValue = 0;
  timeUnitParam.enumOptions = {"Second", "Minute", "Millisecond"};
  timeUnitParam.category = ParameterCategory::BASIC;
  timeUnitParam.required = true;
  fluorocarbonProcess.parameters.push_back(timeUnitParam);

  auto factory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                    vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    int lengthUnit = registry.getParameter<int>(params, "LengthUnit", 0);
    int timeUnit = registry.getParameter<int>(params, "TimeUnit", 0);

    switch (lengthUnit) {
    case 0:
      viennaps::units::Length::setUnit(viennaps::units::Length::NANOMETER);
      break;
    case 1:
      viennaps::units::Length::setUnit(viennaps::units::Length::MICROMETER);
      break;
    case 2:
      viennaps::units::Length::setUnit(viennaps::units::Length::MILLIMETER);
      break;
    case 3:
      viennaps::units::Length::setUnit(viennaps::units::Length::CENTIMETER);
      break;
    case 4:
      viennaps::units::Length::setUnit(viennaps::units::Length::METER);
      break;
    case 5:
      viennaps::units::Length::setUnit(viennaps::units::Length::ANGSTROM);
      break;
    default:
      viennaps::units::Length::setUnit(viennaps::units::Length::NANOMETER);
      break;
    }

    switch (timeUnit) {
    case 0:
      viennaps::units::Time::setUnit(viennaps::units::Time::SECOND);
      break;
    case 1:
      viennaps::units::Time::setUnit(viennaps::units::Time::MINUTE);
      break;
    case 2:
      viennaps::units::Time::setUnit(viennaps::units::Time::MILLISECOND);
      break;
    default:
      viennaps::units::Time::setUnit(viennaps::units::Time::SECOND);
      break;
    }

    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 10.0);
    double ionFlux = registry.getParameter<double>(params, "IonFlux", 56.0);
    double etchantFlux =
        registry.getParameter<double>(params, "EtchantFlux", 500.0);
    double polymerFlux =
        registry.getParameter<double>(params, "PolymerFlux", 100.0);
    double meanEnergy =
        registry.getParameter<double>(params, "MeanEnergy", 100.0);
    double sigmaEnergy =
        registry.getParameter<double>(params, "SigmaEnergy", 10.0);
    double temperature =
        registry.getParameter<double>(params, "Temperature", 300.0);
    double ionExponent =
        registry.getParameter<double>(params, "IonExponent", 500.0);
    double deltaP = registry.getParameter<double>(params, "DeltaP", 1.0);
    double etchStopDepth = registry.getParameter<double>(
        params, "EtchStopDepth", std::numeric_limits<NumericType>::lowest());
    double k_ie = registry.getParameter<double>(params, "K_IE", 2.0);
    double k_ev = registry.getParameter<double>(params, "K_EV", 2.0);
    double beta_p = registry.getParameter<double>(params, "BetaP", 0.26);
    double beta_e = registry.getParameter<double>(params, "BetaE", 0.9);

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;

          viennaps::FluorocarbonParameters<NumericType> fluoroParams;
          fluoroParams.ionFlux = ionFlux;
          fluoroParams.etchantFlux = etchantFlux;
          fluoroParams.polyFlux = polymerFlux;
          fluoroParams.Ions.meanEnergy = meanEnergy;
          fluoroParams.Ions.sigmaEnergy = sigmaEnergy;
          fluoroParams.Ions.exponent = ionExponent;
          fluoroParams.delta_p = deltaP;
          fluoroParams.etchStopDepth = etchStopDepth;
          fluoroParams.temperature = temperature;
          fluoroParams.k_ie = k_ie;
          fluoroParams.k_ev = k_ev;
          // beta_p and beta_e are per-material parameters
          viennaps::FluorocarbonParameters<NumericType>::MaterialParameters
              matParams;
          matParams.beta_p = beta_p;
          matParams.beta_e = beta_e;
          fluoroParams.addMaterial(matParams);

          auto model = viennaps::SmartPointer<viennaps::FluorocarbonEtching<
              NumericType, Dim>>::New(fluoroParams);

          VPSLOG_DEBUG(nullptr, "Ion flux: ", ionFlux, " (1e15 /cm²/s)");
          VPSLOG_DEBUG(nullptr, "Etchant flux: ", etchantFlux,
                       " (1e15 /cm²/s)");
          VPSLOG_DEBUG(nullptr, "Polymer flux: ", polymerFlux,
                       " (1e15 /cm²/s)");
          VPSLOG_DEBUG(nullptr, "Mean ion energy: ", meanEnergy, " eV");
          VPSLOG_DEBUG(nullptr, "Temperature: ", temperature, " K");
          if (etchStopDepth > std::numeric_limits<NumericType>::lowest()) {
            VPSLOG_DEBUG(nullptr, "Etch stop depth: ", etchStopDepth, " nm");
          }

          viennaps::Process<NumericType, Dim> process;
          process.setDomain(psDomain);
          process.setProcessModel(model);
          {
            viennaps::RayTracingParameters rayParams;
            rayParams.raysPerPoint = static_cast<unsigned>(
                registry.getParameter<int>(params, "NumRaysPerPoint", 1000));
            process.setParameters(rayParams);
          }

#ifndef MULTI_STEP
          process.setProcessDuration(processTime);
          process.apply();
#else
                const int numSteps = 20;
                const double stepTime = processTime / numSteps;

                process.setProcessDuration(stepTime);
                for (int step = 0; step < numSteps; ++step) {
                    process.apply();
                }
#endif

          ViennaPSModels::convertToVTK<Dim>(psDomain, output, params);
        });
  };

  fluorocarbonProcess.parameters.push_back(makeNumRaysPerPointParam());

  vtkViennaPSModelRegistry::getInstance().registerProcessModel(
      "FluorocarbonEtching", fluorocarbonProcess, factory);
}

void registerCF4O2EtchingProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata cf4o2Process;
  cf4o2Process.className = "CF4O2Etching";
  cf4o2Process.displayName = "CF4/O2 Plasma Etching";
  cf4o2Process.description = "Advanced plasma etching model with CF4/O2 "
                             "chemistry for Si and SiGe etching";
  cf4o2Process.type = ModelType::SIMULATION;

  ParameterMetadata ionFluxParam;
  ionFluxParam.name = "IonFlux";
  ionFluxParam.displayName = "Ion Flux";
  ionFluxParam.documentation = "Ion flux (1e15 /cm²/s)";
  ionFluxParam.type = ParameterType::DOUBLE;
  ionFluxParam.defaultValue = 12.0;
  ionFluxParam.minValue = 0.0;
  ionFluxParam.maxValue = 100.0;
  ionFluxParam.category = ParameterCategory::BASIC;
  ionFluxParam.unit = "";
  ionFluxParam.stepSize = 1.0;
  ionFluxParam.required = true;
  cf4o2Process.parameters.push_back(ionFluxParam);

  ParameterMetadata etchantFluxParam;
  etchantFluxParam.name = "EtchantFlux";
  etchantFluxParam.displayName = "Etchant (F) Flux";
  etchantFluxParam.documentation = "Fluorine etchant flux (1e15 /cm²/s)";
  etchantFluxParam.type = ParameterType::DOUBLE;
  etchantFluxParam.defaultValue = 1800.0;
  etchantFluxParam.minValue = 0.0;
  etchantFluxParam.maxValue = 10000.0;
  etchantFluxParam.category = ParameterCategory::BASIC;
  etchantFluxParam.unit = "";
  etchantFluxParam.stepSize = 100.0;
  etchantFluxParam.required = true;
  cf4o2Process.parameters.push_back(etchantFluxParam);

  ParameterMetadata oxygenFluxParam;
  oxygenFluxParam.name = "OxygenFlux";
  oxygenFluxParam.displayName = "Oxygen Flux";
  oxygenFluxParam.documentation = "Oxygen flux (1e15 /cm²/s)";
  oxygenFluxParam.type = ParameterType::DOUBLE;
  oxygenFluxParam.defaultValue = 100.0;
  oxygenFluxParam.minValue = 0.0;
  oxygenFluxParam.maxValue = 1000.0;
  oxygenFluxParam.category = ParameterCategory::BASIC;
  oxygenFluxParam.unit = "";
  oxygenFluxParam.stepSize = 10.0;
  oxygenFluxParam.required = true;
  cf4o2Process.parameters.push_back(oxygenFluxParam);

  ParameterMetadata polymerFluxParam;
  polymerFluxParam.name = "PolymerFlux";
  polymerFluxParam.displayName = "Polymer Flux";
  polymerFluxParam.documentation = "Polymer flux (1e15 /cm²/s)";
  polymerFluxParam.type = ParameterType::DOUBLE;
  polymerFluxParam.defaultValue = 100.0;
  polymerFluxParam.minValue = 0.0;
  polymerFluxParam.maxValue = 1000.0;
  polymerFluxParam.category = ParameterCategory::BASIC;
  polymerFluxParam.unit = "";
  polymerFluxParam.stepSize = 10.0;
  polymerFluxParam.required = true;
  cf4o2Process.parameters.push_back(polymerFluxParam);

  ParameterMetadata meanEnergyParam;
  meanEnergyParam.name = "MeanEnergy";
  meanEnergyParam.displayName = "Mean Ion Energy";
  meanEnergyParam.documentation = "Mean energy of ions";
  meanEnergyParam.type = ParameterType::DOUBLE;
  meanEnergyParam.defaultValue = 100.0;
  meanEnergyParam.minValue = 10.0;
  meanEnergyParam.maxValue = 500.0;
  meanEnergyParam.category = ParameterCategory::BASIC;
  meanEnergyParam.unit = "";
  meanEnergyParam.stepSize = 10.0;
  meanEnergyParam.required = true;
  cf4o2Process.parameters.push_back(meanEnergyParam);

  ParameterMetadata sigmaEnergyParam;
  sigmaEnergyParam.name = "SigmaEnergy";
  sigmaEnergyParam.displayName = "Ion Energy Sigma";
  sigmaEnergyParam.documentation =
      "Standard deviation of ion energy distribution";
  sigmaEnergyParam.type = ParameterType::DOUBLE;
  sigmaEnergyParam.defaultValue = 10.0;
  sigmaEnergyParam.minValue = 0.1;
  sigmaEnergyParam.maxValue = 100.0;
  sigmaEnergyParam.category = ParameterCategory::BASIC;
  sigmaEnergyParam.unit = "";
  sigmaEnergyParam.stepSize = 1.0;
  sigmaEnergyParam.required = true;
  cf4o2Process.parameters.push_back(sigmaEnergyParam);

  ParameterMetadata ionExponentParam;
  ionExponentParam.name = "IonExponent";
  ionExponentParam.displayName = "Ion Angular Distribution";
  ionExponentParam.documentation = "Exponent for ion angular distribution";
  ionExponentParam.type = ParameterType::DOUBLE;
  ionExponentParam.defaultValue = 300.0;
  ionExponentParam.minValue = 1.0;
  ionExponentParam.maxValue = 1000.0;
  ionExponentParam.category = ParameterCategory::ADVANCED;
  ionExponentParam.unit = "";
  ionExponentParam.stepSize = 50.0;
  ionExponentParam.required = false;
  cf4o2Process.parameters.push_back(ionExponentParam);

  ParameterMetadata oxySputterParam;
  oxySputterParam.name = "OxygenSputterYield";
  oxySputterParam.displayName = "Oxygen Sputter Yield";
  oxySputterParam.documentation = "Ion-enhanced oxygen sputtering yield";
  oxySputterParam.type = ParameterType::DOUBLE;
  oxySputterParam.defaultValue = 2.0;
  oxySputterParam.minValue = 0.0;
  oxySputterParam.maxValue = 10.0;
  oxySputterParam.category = ParameterCategory::ADVANCED;
  oxySputterParam.unit = "";
  oxySputterParam.stepSize = 0.5;
  oxySputterParam.required = false;
  cf4o2Process.parameters.push_back(oxySputterParam);

  ParameterMetadata polySputterParam;
  polySputterParam.name = "PolymerSputterYield";
  polySputterParam.displayName = "Polymer Sputter Yield";
  polySputterParam.documentation = "Ion-enhanced polymer sputtering yield";
  polySputterParam.type = ParameterType::DOUBLE;
  polySputterParam.defaultValue = 2.0;
  polySputterParam.minValue = 0.0;
  polySputterParam.maxValue = 10.0;
  polySputterParam.category = ParameterCategory::ADVANCED;
  polySputterParam.unit = "";
  polySputterParam.stepSize = 0.5;
  polySputterParam.required = false;
  cf4o2Process.parameters.push_back(polySputterParam);

  ParameterMetadata etchStopParam;
  etchStopParam.name = "EtchStopDepth";
  etchStopParam.displayName = "Etch Stop Depth";
  etchStopParam.documentation =
      "Z-coordinate to stop etching. Leave at minimum for no stop.";
  etchStopParam.type = ParameterType::DOUBLE;
  etchStopParam.defaultValue = std::numeric_limits<double>::lowest();
  etchStopParam.minValue = std::numeric_limits<double>::lowest();
  etchStopParam.maxValue = 0.0;
  etchStopParam.category = ParameterCategory::ADVANCED;
  etchStopParam.unit = "";
  etchStopParam.stepSize = 10.0;
  etchStopParam.required = false;
  cf4o2Process.parameters.push_back(etchStopParam);

  ParameterMetadata siGeXParam;
  siGeXParam.name = "SiGeFraction";
  siGeXParam.displayName = "SiGe Germanium Fraction";
  siGeXParam.documentation = "Germanium fraction in SiGe material (0-1)";
  siGeXParam.type = ParameterType::DOUBLE;
  siGeXParam.defaultValue = 0.3;
  siGeXParam.minValue = 0.0;
  siGeXParam.maxValue = 1.0;
  siGeXParam.category = ParameterCategory::ADVANCED;
  siGeXParam.unit = "";
  siGeXParam.stepSize = 0.05;
  siGeXParam.required = false;
  cf4o2Process.parameters.push_back(siGeXParam);

  ParameterMetadata lengthUnitParam;
  lengthUnitParam.name = "LengthUnit";
  lengthUnitParam.displayName = "Length Unit";
  lengthUnitParam.documentation = "Unit for length measurements";
  lengthUnitParam.type = ParameterType::ENUM;
  lengthUnitParam.defaultValue = 0;
  lengthUnitParam.category = ParameterCategory::BASIC;
  lengthUnitParam.required = true;
  lengthUnitParam.enumOptions = {"Nanometer",  "Micrometer", "Millimeter",
                                 "Centimeter", "Meter",      "Angstrom"};
  cf4o2Process.parameters.push_back(lengthUnitParam);

  ParameterMetadata timeUnitParam;
  timeUnitParam.name = "TimeUnit";
  timeUnitParam.displayName = "Time Unit";
  timeUnitParam.documentation = "Unit for time measurements";
  timeUnitParam.type = ParameterType::ENUM;
  timeUnitParam.defaultValue = 0;
  timeUnitParam.enumOptions = {"Second", "Minute", "Millisecond"};
  timeUnitParam.category = ParameterCategory::BASIC;
  timeUnitParam.required = true;
  cf4o2Process.parameters.push_back(timeUnitParam);

  auto factory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                    vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    int lengthUnit = registry.getParameter<int>(params, "LengthUnit", 0);
    int timeUnit = registry.getParameter<int>(params, "TimeUnit", 0);

    switch (lengthUnit) {
    case 0:
      viennaps::units::Length::setUnit(viennaps::units::Length::NANOMETER);
      break;
    case 1:
      viennaps::units::Length::setUnit(viennaps::units::Length::MICROMETER);
      break;
    case 2:
      viennaps::units::Length::setUnit(viennaps::units::Length::MILLIMETER);
      break;
    case 3:
      viennaps::units::Length::setUnit(viennaps::units::Length::CENTIMETER);
      break;
    case 4:
      viennaps::units::Length::setUnit(viennaps::units::Length::METER);
      break;
    case 5:
      viennaps::units::Length::setUnit(viennaps::units::Length::ANGSTROM);
      break;
    default:
      viennaps::units::Length::setUnit(viennaps::units::Length::NANOMETER);
      break;
    }

    switch (timeUnit) {
    case 0:
      viennaps::units::Time::setUnit(viennaps::units::Time::SECOND);
      break;
    case 1:
      viennaps::units::Time::setUnit(viennaps::units::Time::MINUTE);
      break;
    case 2:
      viennaps::units::Time::setUnit(viennaps::units::Time::MILLISECOND);
      break;
    default:
      viennaps::units::Time::setUnit(viennaps::units::Time::SECOND);
      break;
    }

    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 10.0);
    double ionFlux = registry.getParameter<double>(params, "IonFlux", 12.0);
    double etchantFlux =
        registry.getParameter<double>(params, "EtchantFlux", 1800.0);
    double oxygenFlux =
        registry.getParameter<double>(params, "OxygenFlux", 100.0);
    double polymerFlux =
        registry.getParameter<double>(params, "PolymerFlux", 100.0);
    double meanEnergy =
        registry.getParameter<double>(params, "MeanEnergy", 100.0);
    double sigmaEnergy =
        registry.getParameter<double>(params, "SigmaEnergy", 10.0);
    double ionExponent =
        registry.getParameter<double>(params, "IonExponent", 500.0);
    double oxySputterYield =
        registry.getParameter<double>(params, "OxygenSputterYield", 3.0);
    double polySputterYield =
        registry.getParameter<double>(params, "PolymerSputterYield", 3.0);
    double etchStopDepth = registry.getParameter<double>(
        params, "EtchStopDepth", std::numeric_limits<NumericType>::lowest());
    double siGeX = registry.getParameter<double>(params, "SiGeFraction", 0.3);

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;

          viennaps::CF4O2Parameters<NumericType> cf4o2Params;
          cf4o2Params.ionFlux = ionFlux;
          cf4o2Params.etchantFlux = etchantFlux;
          cf4o2Params.oxygenFlux = oxygenFlux;
          cf4o2Params.polymerFlux = polymerFlux;
          cf4o2Params.Ions.meanEnergy = meanEnergy;
          cf4o2Params.Ions.sigmaEnergy = sigmaEnergy;
          cf4o2Params.Ions.exponent = ionExponent;
          cf4o2Params.Passivation.A_O_ie = oxySputterYield;
          cf4o2Params.Passivation.A_C_ie = polySputterYield;
          cf4o2Params.etchStopDepth = etchStopDepth;
          cf4o2Params.SiGe.x = siGeX;

          auto model = viennaps::SmartPointer<
              viennaps::CF4O2Etching<NumericType, Dim>>::New(cf4o2Params);

          VPSLOG_DEBUG(nullptr, "Ion flux: ", ionFlux, " (1e15 /cm²/s)");
          VPSLOG_DEBUG(nullptr, "Etchant flux: ", etchantFlux,
                       " (1e15 /cm²/s)");
          VPSLOG_DEBUG(nullptr, "Oxygen flux: ", oxygenFlux, " (1e15 /cm²/s)");
          VPSLOG_DEBUG(nullptr, "Polymer flux: ", polymerFlux,
                       " (1e15 /cm²/s)");
          VPSLOG_DEBUG(nullptr, "Mean ion energy: ", meanEnergy, " eV");
          VPSLOG_DEBUG(nullptr, "Ion energy sigma: ", sigmaEnergy, " eV");
          if (etchStopDepth > std::numeric_limits<NumericType>::lowest()) {
            VPSLOG_DEBUG(nullptr, "Etch stop depth: ", etchStopDepth, " nm");
          }

          viennaps::Process<NumericType, Dim> process;
          process.setDomain(psDomain);
          process.setProcessModel(model);
          {
            viennaps::RayTracingParameters rayParams;
            rayParams.raysPerPoint = static_cast<unsigned>(
                registry.getParameter<int>(params, "NumRaysPerPoint", 1000));
            process.setParameters(rayParams);
          }

#ifndef MULTI_STEP
          process.setProcessDuration(processTime);
          process.apply();
#else
                const int numSteps = 20;
                const double stepTime = processTime / numSteps;

                process.setProcessDuration(stepTime);
                for (int step = 0; step < numSteps; ++step) {
                    process.apply();
                }
#endif

          ViennaPSModels::convertToVTK<Dim>(psDomain, output, params);
        });
  };

  cf4o2Process.parameters.push_back(makeNumRaysPerPointParam());

  vtkViennaPSModelRegistry::getInstance().registerProcessModel(
      "CF4O2Etching", cf4o2Process, factory);
}

void registerFaradayCageEtchingProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata faradayCageProcess;
  faradayCageProcess.className = "FaradayCageEtching";
  faradayCageProcess.displayName = "Faraday Cage Etching";
  faradayCageProcess.description =
      "Ion beam etching through a Faraday cage with two-directional ion source";
  faradayCageProcess.type = ModelType::SIMULATION;

  ParameterMetadata cageAngleParam;
  cageAngleParam.name = "CageAngle";
  cageAngleParam.displayName = "Cage Angle";
  cageAngleParam.documentation = "Faraday cage deflection angle (degrees)";
  cageAngleParam.type = ParameterType::DOUBLE;
  cageAngleParam.defaultValue = 0.0;
  cageAngleParam.minValue = 0.0;
  cageAngleParam.maxValue = 90.0;
  cageAngleParam.category = ParameterCategory::BASIC;
  cageAngleParam.unit = "°";
  cageAngleParam.stepSize = 1.0;
  cageAngleParam.required = true;
  faradayCageProcess.parameters.push_back(cageAngleParam);

  ParameterMetadata planeRateParam;
  planeRateParam.name = "PlaneWaferRate";
  planeRateParam.displayName = "Plane Wafer Rate";
  planeRateParam.documentation = "Etching rate on a flat wafer surface";
  planeRateParam.type = ParameterType::DOUBLE;
  planeRateParam.defaultValue = 1.0;
  planeRateParam.minValue = 0.0;
  planeRateParam.maxValue = 10.0;
  planeRateParam.category = ParameterCategory::BASIC;
  planeRateParam.unit = "";
  planeRateParam.stepSize = 0.1;
  planeRateParam.required = true;
  faradayCageProcess.parameters.push_back(planeRateParam);

  ParameterMetadata meanEnergyParam;
  meanEnergyParam.name = "MeanEnergy";
  meanEnergyParam.displayName = "Mean Ion Energy";
  meanEnergyParam.documentation = "Mean energy of ions";
  meanEnergyParam.type = ParameterType::DOUBLE;
  meanEnergyParam.defaultValue = 250.0;
  meanEnergyParam.minValue = 50.0;
  meanEnergyParam.maxValue = 1000.0;
  meanEnergyParam.category = ParameterCategory::BASIC;
  meanEnergyParam.unit = "";
  meanEnergyParam.stepSize = 10.0;
  meanEnergyParam.required = true;
  faradayCageProcess.parameters.push_back(meanEnergyParam);

  ParameterMetadata sigmaEnergyParam;
  sigmaEnergyParam.name = "SigmaEnergy";
  sigmaEnergyParam.displayName = "Ion Energy Sigma";
  sigmaEnergyParam.documentation =
      "Standard deviation of ion energy distribution";
  sigmaEnergyParam.type = ParameterType::DOUBLE;
  sigmaEnergyParam.defaultValue = 10.0;
  sigmaEnergyParam.minValue = 0.1;
  sigmaEnergyParam.maxValue = 100.0;
  sigmaEnergyParam.category = ParameterCategory::BASIC;
  sigmaEnergyParam.unit = "";
  sigmaEnergyParam.stepSize = 1.0;
  sigmaEnergyParam.required = true;
  faradayCageProcess.parameters.push_back(sigmaEnergyParam);

  ParameterMetadata thresholdParam;
  thresholdParam.name = "ThresholdEnergy";
  thresholdParam.displayName = "Threshold Energy";
  thresholdParam.documentation = "Minimum energy for etching";
  thresholdParam.type = ParameterType::DOUBLE;
  thresholdParam.defaultValue = 20.0;
  thresholdParam.minValue = 0.0;
  thresholdParam.maxValue = 100.0;
  thresholdParam.category = ParameterCategory::BASIC;
  thresholdParam.unit = "";
  thresholdParam.stepSize = 1.0;
  thresholdParam.required = true;
  faradayCageProcess.parameters.push_back(thresholdParam);

  ParameterMetadata tiltAngleParam;
  tiltAngleParam.name = "TiltAngle";
  tiltAngleParam.displayName = "Beam Tilt Angle";
  tiltAngleParam.documentation =
      "Ion beam tilt angle from surface normal (degrees)";
  tiltAngleParam.type = ParameterType::DOUBLE;
  tiltAngleParam.defaultValue = 0.0;
  tiltAngleParam.minValue = -89.0;
  tiltAngleParam.maxValue = 89.0;
  tiltAngleParam.category = ParameterCategory::BASIC;
  tiltAngleParam.unit = "°";
  tiltAngleParam.stepSize = 1.0;
  tiltAngleParam.required = true;
  faradayCageProcess.parameters.push_back(tiltAngleParam);

  ParameterMetadata maskParam;
  maskParam.name = "MaskMaterials";
  maskParam.displayName = "Materials to mask";
  maskParam.documentation = "Select mask materials that won't be etched";
  maskParam.type = ParameterType::MATERIAL_LIST;
  maskParam.defaultValue = MaterialListValue{{1}};
  maskParam.category = ParameterCategory::BASIC;
  maskParam.required = true;

  auto materialNames = getAllMaterialNames();
  maskParam.enumOptions = materialNames;
  for (size_t i = 0; i < materialNames.size(); ++i) {
    maskParam.materialMap[static_cast<int>(i)] = materialNames[i];
  }
  faradayCageProcess.parameters.push_back(maskParam);

  ParameterMetadata ionExponentParam;
  ionExponentParam.name = "IonExponent";
  ionExponentParam.displayName = "Ion Angular Distribution";
  ionExponentParam.documentation = "Exponent for ion angular distribution";
  ionExponentParam.type = ParameterType::DOUBLE;
  ionExponentParam.defaultValue = 100.0;
  ionExponentParam.minValue = 1.0;
  ionExponentParam.maxValue = 1000.0;
  ionExponentParam.category = ParameterCategory::ADVANCED;
  ionExponentParam.unit = "";
  ionExponentParam.stepSize = 10.0;
  ionExponentParam.required = false;
  faradayCageProcess.parameters.push_back(ionExponentParam);

  ParameterMetadata nlParam;
  nlParam.name = "NL";
  nlParam.displayName = "Reflection Parameter n_l";
  nlParam.documentation = "Parameter for energy reflection model";
  nlParam.type = ParameterType::DOUBLE;
  nlParam.defaultValue = 10.0;
  nlParam.minValue = 1.0;
  nlParam.maxValue = 100.0;
  nlParam.category = ParameterCategory::ADVANCED;
  nlParam.unit = "";
  nlParam.stepSize = 1.0;
  nlParam.required = false;
  faradayCageProcess.parameters.push_back(nlParam);

  ParameterMetadata inflectAngleParam;
  inflectAngleParam.name = "InflectAngle";
  inflectAngleParam.displayName = "Inflection Angle";
  inflectAngleParam.documentation =
      "Angle for energy reflection inflection point (degrees)";
  inflectAngleParam.type = ParameterType::DOUBLE;
  inflectAngleParam.defaultValue = 89.0;
  inflectAngleParam.minValue = 45.0;
  inflectAngleParam.maxValue = 90.0;
  inflectAngleParam.category = ParameterCategory::ADVANCED;
  inflectAngleParam.unit = "°";
  inflectAngleParam.stepSize = 1.0;
  inflectAngleParam.required = false;
  faradayCageProcess.parameters.push_back(inflectAngleParam);

  ParameterMetadata minAngleParam;
  minAngleParam.name = "MinAngle";
  minAngleParam.displayName = "Minimum Reflection Angle";
  minAngleParam.documentation =
      "Minimum angle for reflected particles (degrees)";
  minAngleParam.type = ParameterType::DOUBLE;
  minAngleParam.defaultValue = 85.0;
  minAngleParam.minValue = 45.0;
  minAngleParam.maxValue = 90.0;
  minAngleParam.category = ParameterCategory::ADVANCED;
  minAngleParam.unit = "°";
  minAngleParam.stepSize = 1.0;
  minAngleParam.required = false;
  faradayCageProcess.parameters.push_back(minAngleParam);

  ParameterMetadata redepositionRateParam;
  redepositionRateParam.name = "RedepositionRate";
  redepositionRateParam.displayName = "Redeposition Rate";
  redepositionRateParam.documentation =
      "Rate of material redeposition (0 = disabled)";
  redepositionRateParam.type = ParameterType::DOUBLE;
  redepositionRateParam.defaultValue = 0.0;
  redepositionRateParam.minValue = 0.0;
  redepositionRateParam.maxValue = 1.0;
  redepositionRateParam.category = ParameterCategory::ADVANCED;
  redepositionRateParam.unit = "";
  redepositionRateParam.stepSize = 0.01;
  redepositionRateParam.required = false;
  faradayCageProcess.parameters.push_back(redepositionRateParam);

  ParameterMetadata redepositionThresholdParam;
  redepositionThresholdParam.name = "RedepositionThreshold";
  redepositionThresholdParam.displayName = "Redeposition Threshold";
  redepositionThresholdParam.documentation =
      "Threshold for material redeposition";
  redepositionThresholdParam.type = ParameterType::DOUBLE;
  redepositionThresholdParam.defaultValue = 0.1;
  redepositionThresholdParam.minValue = 0.0;
  redepositionThresholdParam.maxValue = 1.0;
  redepositionThresholdParam.category = ParameterCategory::ADVANCED;
  redepositionThresholdParam.unit = "";
  redepositionThresholdParam.stepSize = 0.01;
  redepositionThresholdParam.required = false;
  faradayCageProcess.parameters.push_back(redepositionThresholdParam);

  ParameterMetadata yieldFunctionParam;
  yieldFunctionParam.name = "YieldFunction";
  yieldFunctionParam.displayName = "Yield Function Type";
  yieldFunctionParam.documentation = "Angular dependence of sputtering yield";
  yieldFunctionParam.type = ParameterType::ENUM;
  yieldFunctionParam.defaultValue = 0;
  yieldFunctionParam.enumOptions = {"Constant", "Cosine", "Cosine Power",
                                    "Custom", "Cos4"};
  yieldFunctionParam.category = ParameterCategory::ADVANCED;
  yieldFunctionParam.required = false;
  faradayCageProcess.parameters.push_back(yieldFunctionParam);

  ParameterMetadata yieldPowerParam;
  yieldPowerParam.name = "YieldPower";
  yieldPowerParam.displayName = "Yield Function Power";
  yieldPowerParam.documentation = "Power for cosine yield function";
  yieldPowerParam.type = ParameterType::DOUBLE;
  yieldPowerParam.defaultValue = 1.0;
  yieldPowerParam.minValue = 0.1;
  yieldPowerParam.maxValue = 5.0;
  yieldPowerParam.category = ParameterCategory::ADVANCED;
  yieldPowerParam.unit = "";
  yieldPowerParam.stepSize = 0.1;
  yieldPowerParam.required = false;
  faradayCageProcess.parameters.push_back(yieldPowerParam);

  ParameterMetadata thetaRMinParam;
  thetaRMinParam.name = "ThetaRMin";
  thetaRMinParam.displayName = "Min Sticking Reflection Angle";
  thetaRMinParam.documentation = "Minimum sticking reflection angle (degrees)";
  thetaRMinParam.type = ParameterType::DOUBLE;
  thetaRMinParam.defaultValue = 70.0;
  thetaRMinParam.minValue = 0.0;
  thetaRMinParam.maxValue = 90.0;
  thetaRMinParam.category = ParameterCategory::ADVANCED;
  thetaRMinParam.unit = "°";
  thetaRMinParam.stepSize = 1.0;
  thetaRMinParam.required = false;
  faradayCageProcess.parameters.push_back(thetaRMinParam);

  ParameterMetadata thetaRMaxParam;
  thetaRMaxParam.name = "ThetaRMax";
  thetaRMaxParam.displayName = "Max Sticking Reflection Angle";
  thetaRMaxParam.documentation = "Maximum sticking reflection angle (degrees)";
  thetaRMaxParam.type = ParameterType::DOUBLE;
  thetaRMaxParam.defaultValue = 90.0;
  thetaRMaxParam.minValue = 0.0;
  thetaRMaxParam.maxValue = 90.0;
  thetaRMaxParam.category = ParameterCategory::ADVANCED;
  thetaRMaxParam.unit = "°";
  thetaRMaxParam.stepSize = 1.0;
  thetaRMaxParam.required = false;
  faradayCageProcess.parameters.push_back(thetaRMaxParam);

  ParameterMetadata rotatingWaferParam;
  rotatingWaferParam.name = "RotatingWafer";
  rotatingWaferParam.displayName = "Rotating Wafer";
  rotatingWaferParam.documentation = "Enable rotating wafer mode";
  rotatingWaferParam.type = ParameterType::BOOLEAN;
  rotatingWaferParam.defaultValue = 0;
  rotatingWaferParam.category = ParameterCategory::ADVANCED;
  rotatingWaferParam.required = false;
  faradayCageProcess.parameters.push_back(rotatingWaferParam);

  ParameterMetadata cos4A1Param;
  cos4A1Param.name = "Cos4_a1";
  cos4A1Param.displayName = "Cos4 Yield a1";
  cos4A1Param.documentation = "Cos4 yield function parameter a1";
  cos4A1Param.type = ParameterType::DOUBLE;
  cos4A1Param.defaultValue = 0.0;
  cos4A1Param.minValue = -100.0;
  cos4A1Param.maxValue = 100.0;
  cos4A1Param.category = ParameterCategory::ADVANCED;
  cos4A1Param.stepSize = 0.1;
  cos4A1Param.required = false;
  faradayCageProcess.parameters.push_back(cos4A1Param);

  ParameterMetadata cos4A2Param;
  cos4A2Param.name = "Cos4_a2";
  cos4A2Param.displayName = "Cos4 Yield a2";
  cos4A2Param.documentation = "Cos4 yield function parameter a2";
  cos4A2Param.type = ParameterType::DOUBLE;
  cos4A2Param.defaultValue = 0.0;
  cos4A2Param.minValue = -100.0;
  cos4A2Param.maxValue = 100.0;
  cos4A2Param.category = ParameterCategory::ADVANCED;
  cos4A2Param.stepSize = 0.1;
  cos4A2Param.required = false;
  faradayCageProcess.parameters.push_back(cos4A2Param);

  ParameterMetadata cos4A3Param;
  cos4A3Param.name = "Cos4_a3";
  cos4A3Param.displayName = "Cos4 Yield a3";
  cos4A3Param.documentation = "Cos4 yield function parameter a3";
  cos4A3Param.type = ParameterType::DOUBLE;
  cos4A3Param.defaultValue = 0.0;
  cos4A3Param.minValue = -100.0;
  cos4A3Param.maxValue = 100.0;
  cos4A3Param.category = ParameterCategory::ADVANCED;
  cos4A3Param.stepSize = 0.1;
  cos4A3Param.required = false;
  faradayCageProcess.parameters.push_back(cos4A3Param);

  ParameterMetadata cos4A4Param;
  cos4A4Param.name = "Cos4_a4";
  cos4A4Param.displayName = "Cos4 Yield a4";
  cos4A4Param.documentation = "Cos4 yield function parameter a4";
  cos4A4Param.type = ParameterType::DOUBLE;
  cos4A4Param.defaultValue = 0.0;
  cos4A4Param.minValue = -100.0;
  cos4A4Param.maxValue = 100.0;
  cos4A4Param.category = ParameterCategory::ADVANCED;
  cos4A4Param.stepSize = 0.1;
  cos4A4Param.required = false;
  faradayCageProcess.parameters.push_back(cos4A4Param);

  auto factory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                    vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 10.0);
    double cageAngle = registry.getParameter<double>(params, "CageAngle", 0.0);
    double planeWaferRate =
        registry.getParameter<double>(params, "PlaneWaferRate", 1.0);
    double meanEnergy =
        registry.getParameter<double>(params, "MeanEnergy", 250.0);
    double sigmaEnergy =
        registry.getParameter<double>(params, "SigmaEnergy", 10.0);
    double thresholdEnergy =
        registry.getParameter<double>(params, "ThresholdEnergy", 20.0);
    double tiltAngle = registry.getParameter<double>(params, "TiltAngle", 0.0);
    double ionExponent =
        registry.getParameter<double>(params, "IonExponent", 100.0);
    double n_l = registry.getParameter<double>(params, "NL", 10.0);
    double inflectAngle =
        registry.getParameter<double>(params, "InflectAngle", 89.0);
    double minAngle = registry.getParameter<double>(params, "MinAngle", 85.0);
    double redepositionRate =
        registry.getParameter<double>(params, "RedepositionRate", 0.0);
    double redepositionThreshold =
        registry.getParameter<double>(params, "RedepositionThreshold", 0.1);
    int yieldFunctionType =
        registry.getParameter<int>(params, "YieldFunction", 0);
    double yieldPower =
        registry.getParameter<double>(params, "YieldPower", 1.0);
    double thetaRMin = registry.getParameter<double>(params, "ThetaRMin", 70.0);
    double thetaRMax = registry.getParameter<double>(params, "ThetaRMax", 90.0);
    bool rotatingWafer =
        registry.getParameter<int>(params, "RotatingWafer", 0) != 0;
    double cos4_a1 = registry.getParameter<double>(params, "Cos4_a1", 0.0);
    double cos4_a2 = registry.getParameter<double>(params, "Cos4_a2", 0.0);
    double cos4_a3 = registry.getParameter<double>(params, "Cos4_a3", 0.0);
    double cos4_a4 = registry.getParameter<double>(params, "Cos4_a4", 0.0);

    std::vector<viennaps::Material> maskMaterials;
    if (params.find("MaskMaterials") != params.end()) {
      auto matList = registry.getParameter<MaterialListValue>(
          params, "MaskMaterials", MaterialListValue{{1}});

      for (int matId : matList.materialIds) {
        auto material = static_cast<viennaps::Material>(matId);
        if (material != viennaps::Material::Undefined) {
          maskMaterials.push_back(material);
        }
      }
    } else {
      maskMaterials.push_back(viennaps::Material::Mask);
    }

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;

          viennaps::FaradayCageParameters<NumericType> fcParams;
          fcParams.cageAngle = cageAngle;
          fcParams.ibeParams.planeWaferRate = planeWaferRate;
          fcParams.ibeParams.meanEnergy = meanEnergy;
          fcParams.ibeParams.sigmaEnergy = sigmaEnergy;
          fcParams.ibeParams.thresholdEnergy = thresholdEnergy;
          fcParams.ibeParams.tiltAngle = tiltAngle;
          fcParams.ibeParams.exponent = ionExponent;
          fcParams.ibeParams.n_l = n_l;
          fcParams.ibeParams.inflectAngle = inflectAngle;
          fcParams.ibeParams.minAngle = minAngle;
          fcParams.ibeParams.redepositionRate = redepositionRate;
          fcParams.ibeParams.redepositionThreshold = redepositionThreshold;
          fcParams.ibeParams.thetaRMin = thetaRMin;
          fcParams.ibeParams.thetaRMax = thetaRMax;
          fcParams.ibeParams.rotatingWafer = rotatingWafer;

          switch (yieldFunctionType) {
          case 0:
            fcParams.ibeParams.yieldFunction = [](NumericType theta) {
              return 1.0;
            };
            break;
          case 1:
            fcParams.ibeParams.yieldFunction = [](NumericType theta) {
              return std::cos(theta);
            };
            break;
          case 2:
            fcParams.ibeParams.yieldFunction = [yieldPower](NumericType theta) {
              return std::pow(std::cos(theta), yieldPower);
            };
            break;
          case 3:
            fcParams.ibeParams.yieldFunction = [](NumericType theta) {
              return 1.0;
            };
            break;
          case 4: // Cos4
            fcParams.ibeParams.cos4Yield.a1 = cos4_a1;
            fcParams.ibeParams.cos4Yield.a2 = cos4_a2;
            fcParams.ibeParams.cos4Yield.a3 = cos4_a3;
            fcParams.ibeParams.cos4Yield.a4 = cos4_a4;
            fcParams.ibeParams.cos4Yield.isDefined = true;
            break;
          default:
            fcParams.ibeParams.yieldFunction = [](NumericType theta) {
              return 1.0;
            };
            break;
          }

          auto model = viennaps::
              SmartPointer<viennaps::FaradayCageEtching<NumericType, Dim>>::New(
                  fcParams, maskMaterials);

          VPSLOG_DEBUG(nullptr, "Cage angle: ", cageAngle, " degrees");
          VPSLOG_DEBUG(nullptr, "Plane wafer rate: ", planeWaferRate, " nm/s");
          VPSLOG_DEBUG(nullptr, "Mean ion energy: ", meanEnergy, " eV");
          VPSLOG_DEBUG(nullptr, "Beam tilt angle: ", tiltAngle, " degrees");
          VPSLOG_DEBUG(nullptr, "Mask materials: ", maskMaterials.size());

          viennaps::Process<NumericType, Dim> process;
          process.setDomain(psDomain);
          process.setProcessModel(model);
          {
            viennaps::RayTracingParameters rayParams;
            rayParams.raysPerPoint = static_cast<unsigned>(
                registry.getParameter<int>(params, "NumRaysPerPoint", 1000));
            process.setParameters(rayParams);
          }

#ifndef MULTI_STEP
          process.setProcessDuration(processTime);
          process.apply();
#else
                const int numSteps = 20;
                const double stepTime = processTime / numSteps;

                process.setProcessDuration(stepTime);
                for (int step = 0; step < numSteps; ++step) {
                    process.apply();
                }
#endif

          ViennaPSModels::convertToVTK<Dim>(psDomain, output, params);
        });
  };

  faradayCageProcess.parameters.push_back(makeNumRaysPerPointParam());

  vtkViennaPSModelRegistry::getInstance().registerProcessModel(
      "FaradayCageEtching", faradayCageProcess, factory);
}

} // anonymous namespace

void ViennaPSModels::initializeEtchingModels() {
  registerWetEtchingProcessModel();
  registerSF6O2EtchingProcessModel();
  registerSF6C4F8EtchingProcessModel();
  registerIonBeamEtchingProcessModel();
  registerHBrO2EtchingProcessModel();
  registerFluorocarbonEtchingProcessModel();
  registerCF4O2EtchingProcessModel();
  registerFaradayCageEtchingProcessModel();
}
