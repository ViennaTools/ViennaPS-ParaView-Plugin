#include "vtkViennaPSBuiltinModels.h"

#include <models/psSelectiveEpitaxy.hpp>
#include <models/psSingleParticleALD.hpp>
#include <models/psTEOSDeposition.hpp>
#include <models/psTEOSPECVD.hpp>

#include <iostream>

namespace {
using NumericType = ViennaPSMeta::NumericType;
constexpr int D = ViennaPSMeta::D;

void registerSelectiveEpitaxyProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata epitaxyProcess;
  epitaxyProcess.className = "SelectiveEpitaxy";
  epitaxyProcess.displayName = "Selective Epitaxy";
  epitaxyProcess.description = "Selective epitaxial growth model with crystal "
                               "orientation-dependent rates";
  epitaxyProcess.type = ModelType::PROCESS;

  ParameterMetadata epitaxyMaterialsParam;
  epitaxyMaterialsParam.name = "EpitaxyMaterials";
  epitaxyMaterialsParam.displayName = "Epitaxy Materials";
  epitaxyMaterialsParam.documentation =
      "Select materials where epitaxial growth is allowed";
  epitaxyMaterialsParam.type = ParameterType::MATERIAL_LIST;
  epitaxyMaterialsParam.defaultValue = MaterialListValue{{0}};
  epitaxyMaterialsParam.category = ParameterCategory::BASIC;
  epitaxyMaterialsParam.required = true;

  auto materialNames = getAllMaterialNames();
  epitaxyMaterialsParam.enumOptions = materialNames;
  for (size_t i = 0; i < materialNames.size(); ++i) {
    epitaxyMaterialsParam.materialMap[static_cast<int>(i)] = materialNames[i];
  }
  epitaxyProcess.parameters.push_back(epitaxyMaterialsParam);

  ParameterMetadata materialRateParam;
  materialRateParam.name = "MaterialRateMultiplier";
  materialRateParam.displayName = "Material Rate Multiplier";
  materialRateParam.documentation =
      "Rate multiplier for selected epitaxy materials";
  materialRateParam.type = ParameterType::DOUBLE;
  materialRateParam.defaultValue = 1.0;
  materialRateParam.minValue = 0.0;
  materialRateParam.maxValue = 10.0;
  materialRateParam.category = ParameterCategory::BASIC;
  materialRateParam.unit = "";
  materialRateParam.stepSize = 0.1;
  materialRateParam.required = true;
  epitaxyProcess.parameters.push_back(materialRateParam);

  ParameterMetadata rate111Param;
  rate111Param.name = "Rate111";
  rate111Param.displayName = "(111) Growth Rate";
  rate111Param.documentation =
      "Epitaxial growth rate for (111) crystal orientation";
  rate111Param.type = ParameterType::DOUBLE;
  rate111Param.defaultValue = 0.5;
  rate111Param.minValue = 0.0;
  rate111Param.maxValue = 100.0;
  rate111Param.category = ParameterCategory::BASIC;
  rate111Param.unit = "";
  rate111Param.stepSize = 0.1;
  rate111Param.required = true;
  epitaxyProcess.parameters.push_back(rate111Param);

  ParameterMetadata rate100Param;
  rate100Param.name = "Rate100";
  rate100Param.displayName = "(100) Growth Rate";
  rate100Param.documentation =
      "Epitaxial growth rate for (100) crystal orientation";
  rate100Param.type = ParameterType::DOUBLE;
  rate100Param.defaultValue = 1.0;
  rate100Param.minValue = 0.0;
  rate100Param.maxValue = 100.0;
  rate100Param.category = ParameterCategory::BASIC;
  rate100Param.unit = "";
  rate100Param.stepSize = 0.1;
  rate100Param.required = true;
  epitaxyProcess.parameters.push_back(rate100Param);

  ParameterMetadata growthMaterialParam;
  growthMaterialParam.name = "GrowthMaterial";
  growthMaterialParam.displayName = "Growth Material";
  growthMaterialParam.documentation =
      "Material to grow epitaxially (e.g., Si, SiGe, Ge)";
  growthMaterialParam.type = ParameterType::ENUM;
  growthMaterialParam.defaultValue = static_cast<int>(viennaps::Material::Si);
  growthMaterialParam.category = ParameterCategory::ADVANCED;
  growthMaterialParam.required = false;

  growthMaterialParam.enumOptions = materialNames;
  for (size_t i = 0; i < materialNames.size(); ++i) {
    growthMaterialParam.materialMap[static_cast<int>(i)] = materialNames[i];
  }
  epitaxyProcess.parameters.push_back(growthMaterialParam);

  auto factory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                    vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 10.0);
    double materialRateMultiplier =
        registry.getParameter<double>(params, "MaterialRateMultiplier", 1.0);
    double rate111 = registry.getParameter<double>(params, "Rate111", 0.5);
    double rate100 = registry.getParameter<double>(params, "Rate100", 1.0);

    viennaps::Material growthMaterial = viennaps::Material::Si;
    if (params.find("GrowthMaterial") != params.end()) {
      int matId = registry.getParameter<int>(
          params, "GrowthMaterial", static_cast<int>(viennaps::Material::Si));
      growthMaterial = static_cast<viennaps::Material>(matId);
    }

    std::vector<std::pair<viennaps::Material, NumericType>> materialRates;
    if (params.find("EpitaxyMaterials") != params.end()) {
      auto matList = registry.getParameter<MaterialListValue>(
          params, "EpitaxyMaterials", MaterialListValue{{0}});

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

          if (psDomain->getLevelSets().size() < 2) {
            VPSLOG_WARNING(
                nullptr,
                "Warning: Selective epitaxy requires at least 2 level sets.");
            VPSLOG_INFO(nullptr, "Adding growth material layer...");

            psDomain->duplicateTopLevelSet(growthMaterial);
          }

          bool topIsEpitaxyMaterial = false;
          auto topMaterial = psDomain->getMaterialMap()->getMaterialAtIdx(
              psDomain->getNumberOfLevelSets() - 1);
          for (const auto &mat : materialRates) {
            if (viennaps::MaterialMap::isMaterial(topMaterial, mat.first)) {
              topIsEpitaxyMaterial = true;
              break;
            }
          }

          if (!topIsEpitaxyMaterial) {
            VPSLOG_WARNING(nullptr,
                           "Warning: Top material is not an epitaxy material.");
            VPSLOG_DEBUG(nullptr, "Adding ",
                         viennaps::MaterialMap::toString(growthMaterial),
                         " as epitaxy layer...");

            bool found = false;
            for (auto &mat : materialRates) {
              if (viennaps::MaterialMap::isMaterial(growthMaterial,
                                                    mat.first)) {
                found = true;
                break;
              }
            }
            if (!found) {
              materialRates.push_back({growthMaterial, materialRateMultiplier});
            }

            psDomain->duplicateTopLevelSet(growthMaterial);
          }

          auto model = viennaps::SmartPointer<
              viennaps::SelectiveEpitaxy<NumericType, Dim>>::New(materialRates,
                                                                 rate111,
                                                                 rate100);

          VPSLOG_DEBUG(nullptr, "Crystal orientation rates:");
          VPSLOG_DEBUG(nullptr, "  (111): ", rate111, " nm/s");
          VPSLOG_DEBUG(nullptr, "  (100): ", rate100, " nm/s");
          VPSLOG_DEBUG(nullptr, "Epitaxy materials: ", materialRates.size());
          for (const auto &mat : materialRates) {
            VPSLOG_DEBUG(nullptr, "  ",
                         viennaps::MaterialMap::toString(mat.first),
                         " (rate multiplier: ", mat.second, ")");
          }
          VPSLOG_DEBUG(nullptr, "Growth material: ",
                       viennaps::MaterialMap::toString(growthMaterial));
          VPSLOG_DEBUG(nullptr, "Process time: ", processTime, " s");

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

  vtkViennaPSModelRegistry::getInstance().registerProcessModel(
      "SelectiveEpitaxy", epitaxyProcess, factory);
}

void registerSingleParticleALDProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata aldProcess;
  aldProcess.className = "SingleParticleALD";
  aldProcess.displayName = "Single Particle ALD";
  aldProcess.description = "Atomic Layer Deposition model with single particle "
                           "species and surface coverage dynamics";
  aldProcess.type = ModelType::PROCESS;

  ParameterMetadata stickingParam;
  stickingParam.name = "StickingProbability";
  stickingParam.displayName = "Sticking Probability";
  stickingParam.documentation = "Particle sticking probability on surface";
  stickingParam.type = ParameterType::DOUBLE;
  stickingParam.defaultValue = 1.0;
  stickingParam.minValue = 0.0;
  stickingParam.maxValue = 1.0;
  stickingParam.category = ParameterCategory::BASIC;
  stickingParam.unit = "";
  stickingParam.stepSize = 0.01;
  stickingParam.required = true;
  aldProcess.parameters.push_back(stickingParam);

  ParameterMetadata numCyclesParam;
  numCyclesParam.name = "NumCycles";
  numCyclesParam.displayName = "Simulation Cycles";
  numCyclesParam.documentation =
      "Number of ALD cycles to simulate in one advection step";
  numCyclesParam.type = ParameterType::INTEGER;
  numCyclesParam.defaultValue = 10;
  numCyclesParam.minValue = 1;
  numCyclesParam.maxValue = 1000;
  numCyclesParam.category = ParameterCategory::BASIC;
  numCyclesParam.unit = "";
  numCyclesParam.stepSize = 1;
  numCyclesParam.required = true;
  aldProcess.parameters.push_back(numCyclesParam);

  ParameterMetadata gpcParam;
  gpcParam.name = "GrowthPerCycle";
  gpcParam.displayName = "Growth Per Cycle";
  gpcParam.documentation = "Film thickness growth per ALD cycle";
  gpcParam.type = ParameterType::DOUBLE;
  gpcParam.defaultValue = 0.1;
  gpcParam.minValue = 0.0;
  gpcParam.maxValue = 1.0;
  gpcParam.category = ParameterCategory::BASIC;
  gpcParam.unit = "";
  gpcParam.stepSize = 0.01;
  gpcParam.required = true;
  aldProcess.parameters.push_back(gpcParam);

  ParameterMetadata totalCyclesParam;
  totalCyclesParam.name = "TotalCycles";
  totalCyclesParam.displayName = "Total Cycles";
  totalCyclesParam.documentation = "Total number of ALD cycles in the process";
  totalCyclesParam.type = ParameterType::INTEGER;
  totalCyclesParam.defaultValue = 100;
  totalCyclesParam.minValue = 1;
  totalCyclesParam.maxValue = 10000;
  totalCyclesParam.category = ParameterCategory::BASIC;
  totalCyclesParam.unit = "";
  totalCyclesParam.stepSize = 10;
  totalCyclesParam.required = true;
  aldProcess.parameters.push_back(totalCyclesParam);

  ParameterMetadata coverageTimeStepParam;
  coverageTimeStepParam.name = "CoverageTimeStep";
  coverageTimeStepParam.displayName = "Coverage Time Step";
  coverageTimeStepParam.documentation =
      "Time step for solving surface coverage equation";
  coverageTimeStepParam.type = ParameterType::DOUBLE;
  coverageTimeStepParam.defaultValue = 0.01;
  coverageTimeStepParam.minValue = 0.0001;
  coverageTimeStepParam.maxValue = 1.0;
  coverageTimeStepParam.category = ParameterCategory::BASIC;
  coverageTimeStepParam.unit = "";
  coverageTimeStepParam.stepSize = 0.001;
  coverageTimeStepParam.required = true;
  aldProcess.parameters.push_back(coverageTimeStepParam);

  ParameterMetadata evFluxParam;
  evFluxParam.name = "EvaporationFlux";
  evFluxParam.displayName = "Evaporation Flux";
  evFluxParam.documentation = "Flux of evaporating species from surface";
  evFluxParam.type = ParameterType::DOUBLE;
  evFluxParam.defaultValue = 0.0;
  evFluxParam.minValue = 0.0;
  evFluxParam.maxValue = 100.0;
  evFluxParam.category = ParameterCategory::ADVANCED;
  evFluxParam.unit = "";
  evFluxParam.stepSize = 0.1;
  evFluxParam.required = false;
  aldProcess.parameters.push_back(evFluxParam);

  ParameterMetadata inFluxParam;
  inFluxParam.name = "IncomingFlux";
  inFluxParam.displayName = "Incoming Flux";
  inFluxParam.documentation = "Incoming particle flux";
  inFluxParam.type = ParameterType::DOUBLE;
  inFluxParam.defaultValue = 1.0;
  inFluxParam.minValue = 0.0;
  inFluxParam.maxValue = 1000.0;
  inFluxParam.category = ParameterCategory::ADVANCED;
  inFluxParam.unit = "";
  inFluxParam.stepSize = 1.0;
  inFluxParam.required = false;
  aldProcess.parameters.push_back(inFluxParam);

  ParameterMetadata s0Param;
  s0Param.name = "SaturationCoverage";
  s0Param.displayName = "Saturation Coverage";
  s0Param.documentation = "Surface saturation coverage density";
  s0Param.type = ParameterType::DOUBLE;
  s0Param.defaultValue = 1.0;
  s0Param.minValue = 0.1;
  s0Param.maxValue = 10.0;
  s0Param.category = ParameterCategory::ADVANCED;
  s0Param.unit = "";
  s0Param.stepSize = 0.1;
  s0Param.required = false;
  aldProcess.parameters.push_back(s0Param);

  ParameterMetadata gasMFPParam;
  gasMFPParam.name = "GasMeanFreePath";
  gasMFPParam.displayName = "Gas Mean Free Path";
  gasMFPParam.documentation =
      "Mean free path of particles in gas phase. 0 = ballistic transport";
  gasMFPParam.type = ParameterType::DOUBLE;
  gasMFPParam.defaultValue = 0.0;
  gasMFPParam.minValue = 0.0;
  gasMFPParam.maxValue = 1000.0;
  gasMFPParam.category = ParameterCategory::ADVANCED;
  gasMFPParam.unit = "";
  gasMFPParam.stepSize = 1.0;
  gasMFPParam.required = false;
  aldProcess.parameters.push_back(gasMFPParam);

  ParameterMetadata depositionMaterialParam;
  depositionMaterialParam.name = "DepositionMaterial";
  depositionMaterialParam.displayName = "Deposition Material";
    depositionMaterialParam.documentation = "Material to deposit (e.g., Al2O3, HfO2, TiO2)";
    depositionMaterialParam.type = ParameterType::STRING;
    depositionMaterialParam.defaultValue = std::string("SiO2");
  depositionMaterialParam.category = ParameterCategory::BASIC;
  depositionMaterialParam.required = false;
  aldProcess.parameters.push_back(depositionMaterialParam);

  auto factory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                    vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    double stickingProbability =
        registry.getParameter<double>(params, "StickingProbability", 1.0);
    int numCycles = registry.getParameter<int>(params, "NumCycles", 10);
    double growthPerCycle =
        registry.getParameter<double>(params, "GrowthPerCycle", 0.1);
    int totalCycles = registry.getParameter<int>(params, "TotalCycles", 100);
    double coverageTimeStep =
        registry.getParameter<double>(params, "CoverageTimeStep", 0.01);
    double evaporationFlux =
        registry.getParameter<double>(params, "EvaporationFlux", 0.0);
    double incomingFlux =
        registry.getParameter<double>(params, "IncomingFlux", 1.0);
    double saturationCoverage =
        registry.getParameter<double>(params, "SaturationCoverage", 1.0);
    double gasMeanFreePath =
        registry.getParameter<double>(params, "GasMeanFreePath", 0.0);

        std::string depositionMaterialName =
            registry.getParameter<std::string>(params, "DepositionMaterial", "SiO2");
        viennaps::Material depositionMaterial =
            ViennaPSMeta::resolveMaterialFromString(depositionMaterialName);

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;

          psDomain->duplicateTopLevelSet(depositionMaterial);

          auto model = viennaps::
              SmartPointer<viennaps::SingleParticleALD<NumericType, Dim>>::New(
                  stickingProbability, numCycles, growthPerCycle, totalCycles,
                  coverageTimeStep, evaporationFlux, incomingFlux,
                  saturationCoverage, gasMeanFreePath);

          VPSLOG_DEBUG(nullptr, "Sticking probability: ", stickingProbability);
          VPSLOG_DEBUG(nullptr, "Growth per cycle: ", growthPerCycle,
                       " nm/cycle");
          VPSLOG_DEBUG(nullptr, "Total cycles: ", totalCycles);
          VPSLOG_DEBUG(nullptr, "Simulation cycles per step: ", numCycles);
          VPSLOG_DEBUG(nullptr, "Coverage time step: ", coverageTimeStep, " s");
          VPSLOG_DEBUG(nullptr, "Incoming flux: ", incomingFlux);
          VPSLOG_DEBUG(nullptr, "Evaporation flux: ", evaporationFlux);
          VPSLOG_DEBUG(nullptr, "Saturation coverage: ", saturationCoverage);
          if (gasMeanFreePath > 0) {
            VPSLOG_DEBUG(nullptr, "Gas mean free path: ", gasMeanFreePath,
                         " nm");
          } else {
            VPSLOG_DEBUG(nullptr, "Transport mode: Ballistic");
          }
          VPSLOG_DEBUG(nullptr, "Deposition material: ",
                       viennaps::MaterialMap::toString(depositionMaterial));

          double totalGrowth = totalCycles * growthPerCycle;
          VPSLOG_DEBUG(nullptr, "Expected total growth: ", totalGrowth, " nm");

          viennaps::Process<NumericType, Dim> process;
          process.setDomain(psDomain);
          process.setProcessModel(model);
          {
            viennaps::RayTracingParameters rayParams;
            rayParams.raysPerPoint = static_cast<unsigned>(
                registry.getParameter<int>(params, "NumRaysPerPoint", 1000));
            process.setParameters(rayParams);
          }

          process.setProcessDuration(
              1.0); // Unit time, actual growth controlled by cycles

          process.apply();

          ViennaPSModels::convertToVTK<Dim>(psDomain, output, params);
        });
  };

  vtkViennaPSModelRegistry::getInstance().registerProcessModel(
      "SingleParticleALD", aldProcess, factory);
}

void registerTEOSPECVDProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata teosPECVDProcess;
  teosPECVDProcess.className = "TEOSPECVD";
  teosPECVDProcess.displayName = "TEOS PECVD";
  teosPECVDProcess.description = "Plasma-enhanced chemical vapor deposition of "
                                 "TEOS with radical and ion contributions";
  teosPECVDProcess.type = ModelType::PROCESS;

  ParameterMetadata radicalStickingParam;
  radicalStickingParam.name = "RadicalSticking";
  radicalStickingParam.displayName = "Radical Sticking Probability";
  radicalStickingParam.documentation =
      "Sticking probability for radical species";
  radicalStickingParam.type = ParameterType::DOUBLE;
  radicalStickingParam.defaultValue = 1.0;
  radicalStickingParam.minValue = 0.0;
  radicalStickingParam.maxValue = 1.0;
  radicalStickingParam.category = ParameterCategory::BASIC;
  radicalStickingParam.unit = "";
  radicalStickingParam.stepSize = 0.01;
  radicalStickingParam.required = true;
  teosPECVDProcess.parameters.push_back(radicalStickingParam);

  ParameterMetadata radicalRateParam;
  radicalRateParam.name = "RadicalRate";
  radicalRateParam.displayName = "Radical Deposition Rate";
  radicalRateParam.documentation = "Deposition rate for radical species";
  radicalRateParam.type = ParameterType::DOUBLE;
  radicalRateParam.defaultValue = 1.0;
  radicalRateParam.minValue = 0.0;
  radicalRateParam.maxValue = 100.0;
  radicalRateParam.category = ParameterCategory::BASIC;
  radicalRateParam.unit = "";
  radicalRateParam.stepSize = 0.1;
  radicalRateParam.required = true;
  teosPECVDProcess.parameters.push_back(radicalRateParam);

  ParameterMetadata ionRateParam;
  ionRateParam.name = "IonRate";
  ionRateParam.displayName = "Ion Deposition Rate";
  ionRateParam.documentation = "Deposition rate for ion species";
  ionRateParam.type = ParameterType::DOUBLE;
  ionRateParam.defaultValue = 1.0;
  ionRateParam.minValue = 0.0;
  ionRateParam.maxValue = 100.0;
  ionRateParam.category = ParameterCategory::BASIC;
  ionRateParam.unit = "";
  ionRateParam.stepSize = 0.1;
  ionRateParam.required = true;
  teosPECVDProcess.parameters.push_back(ionRateParam);

  ParameterMetadata ionExponentParam;
  ionExponentParam.name = "IonExponent";
  ionExponentParam.displayName = "Ion Angular Distribution";
  ionExponentParam.documentation = "Exponent for ion angular distribution";
  ionExponentParam.type = ParameterType::DOUBLE;
  ionExponentParam.defaultValue = 100.0;
  ionExponentParam.minValue = 1.0;
  ionExponentParam.maxValue = 1000.0;
  ionExponentParam.category = ParameterCategory::BASIC;
  ionExponentParam.unit = "";
  ionExponentParam.stepSize = 10.0;
  ionExponentParam.required = true;
  teosPECVDProcess.parameters.push_back(ionExponentParam);

  ParameterMetadata ionStickingParam;
  ionStickingParam.name = "IonSticking";
  ionStickingParam.displayName = "Ion Sticking Probability";
  ionStickingParam.documentation = "Sticking probability for ion species";
  ionStickingParam.type = ParameterType::DOUBLE;
  ionStickingParam.defaultValue = 1.0;
  ionStickingParam.minValue = 0.0;
  ionStickingParam.maxValue = 1.0;
  ionStickingParam.category = ParameterCategory::ADVANCED;
  ionStickingParam.unit = "";
  ionStickingParam.stepSize = 0.01;
  ionStickingParam.required = false;
  teosPECVDProcess.parameters.push_back(ionStickingParam);

  ParameterMetadata radicalOrderParam;
  radicalOrderParam.name = "RadicalReactionOrder";
  radicalOrderParam.displayName = "Radical Reaction Order";
  radicalOrderParam.documentation = "Reaction order for radical species";
  radicalOrderParam.type = ParameterType::DOUBLE;
  radicalOrderParam.defaultValue = 1.0;
  radicalOrderParam.minValue = 0.0;
  radicalOrderParam.maxValue = 5.0;
  radicalOrderParam.category = ParameterCategory::ADVANCED;
  radicalOrderParam.unit = "";
  radicalOrderParam.stepSize = 0.1;
  radicalOrderParam.required = false;
  teosPECVDProcess.parameters.push_back(radicalOrderParam);

  ParameterMetadata ionOrderParam;
  ionOrderParam.name = "IonReactionOrder";
  ionOrderParam.displayName = "Ion Reaction Order";
  ionOrderParam.documentation = "Reaction order for ion species";
  ionOrderParam.type = ParameterType::DOUBLE;
  ionOrderParam.defaultValue = 1.0;
  ionOrderParam.minValue = 0.0;
  ionOrderParam.maxValue = 5.0;
  ionOrderParam.category = ParameterCategory::ADVANCED;
  ionOrderParam.unit = "";
  ionOrderParam.stepSize = 0.1;
  ionOrderParam.required = false;
  teosPECVDProcess.parameters.push_back(ionOrderParam);

  ParameterMetadata ionMinAngleParam;
  ionMinAngleParam.name = "IonMinAngle";
  ionMinAngleParam.displayName = "Ion Minimum Reflection Angle";
  ionMinAngleParam.documentation = "Minimum angle for ion reflection (degrees)";
  ionMinAngleParam.type = ParameterType::DOUBLE;
  ionMinAngleParam.defaultValue = 85.0;
  ionMinAngleParam.minValue = 0.0;
  ionMinAngleParam.maxValue = 90.0;
  ionMinAngleParam.category = ParameterCategory::ADVANCED;
  ionMinAngleParam.unit = "°";
  ionMinAngleParam.stepSize = 1.0;
  ionMinAngleParam.required = false;
  teosPECVDProcess.parameters.push_back(ionMinAngleParam);

  ParameterMetadata depositionMaterialParam;
  depositionMaterialParam.name = "DepositionMaterial";
  depositionMaterialParam.displayName = "Deposition Material";
    depositionMaterialParam.documentation = "Material to deposit (typically SiO2 for TEOS)";
    depositionMaterialParam.type = ParameterType::STRING;
    depositionMaterialParam.defaultValue = std::string("SiO2");
  depositionMaterialParam.category = ParameterCategory::ADVANCED;
  depositionMaterialParam.required = false;
  teosPECVDProcess.parameters.push_back(depositionMaterialParam);

  auto factory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                    vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 10.0);
    double radicalSticking =
        registry.getParameter<double>(params, "RadicalSticking", 1.0);
    double radicalRate =
        registry.getParameter<double>(params, "RadicalRate", 1.0);
    double ionRate = registry.getParameter<double>(params, "IonRate", 1.0);
    double ionExponent =
        registry.getParameter<double>(params, "IonExponent", 100.0);
    double ionSticking =
        registry.getParameter<double>(params, "IonSticking", 1.0);
    double radicalOrder =
        registry.getParameter<double>(params, "RadicalReactionOrder", 1.0);
    double ionOrder =
        registry.getParameter<double>(params, "IonReactionOrder", 1.0);
    double ionMinAngle =
        registry.getParameter<double>(params, "IonMinAngle", 0.0);

        std::string depositionMaterialName =
            registry.getParameter<std::string>(params, "DepositionMaterial", "SiO2");
        viennaps::Material depositionMaterial =
            ViennaPSMeta::resolveMaterialFromString(depositionMaterialName);

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;

          psDomain->duplicateTopLevelSet(depositionMaterial);

          auto model =
              viennaps::SmartPointer<viennaps::TEOSPECVD<NumericType, Dim>>::
                  New(radicalSticking, radicalRate, ionRate, ionExponent,
                      ionSticking, radicalOrder, ionOrder, ionMinAngle);

          VPSLOG_DEBUG(nullptr, "Radicals - Sticking: ", radicalSticking,
                       ", Rate: ", radicalRate, " nm/s",
                       ", Order: ", radicalOrder);
          VPSLOG_DEBUG(nullptr, "Ions - Sticking: ", ionSticking,
                       ", Rate: ", ionRate, " nm/s", ", Order: ", ionOrder,
                       ", Exponent: ", ionExponent);
          VPSLOG_DEBUG(nullptr, "Ion minimum reflection angle: ", ionMinAngle,
                       " rad");
          VPSLOG_DEBUG(nullptr, "Deposition material: ",
                       viennaps::MaterialMap::toString(depositionMaterial));
          VPSLOG_DEBUG(nullptr, "Process time: ", processTime, " s");

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

  vtkViennaPSModelRegistry::getInstance().registerProcessModel(
      "TEOSPECVD", teosPECVDProcess, factory);
}

void registerTEOSDepositionProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata teosProcess;
  teosProcess.className = "TEOSDeposition";
  teosProcess.displayName = "TEOS Deposition";
  teosProcess.description =
      "Tetraethyl orthosilicate (TEOS) deposition model for SiO2 film growth";
  teosProcess.type = ModelType::PROCESS;

  ParameterMetadata modelTypeParam;
  modelTypeParam.name = "ModelType";
  modelTypeParam.displayName = "Model Type";
  modelTypeParam.documentation = "Single or dual particle model";
  modelTypeParam.type = ParameterType::ENUM;
  modelTypeParam.defaultValue = 0;
  modelTypeParam.enumOptions = {"Single Particle", "Dual Particle"};
  modelTypeParam.category = ParameterCategory::BASIC;
  modelTypeParam.required = true;
  teosProcess.parameters.push_back(modelTypeParam);

  ParameterMetadata stickingP1Param;
  stickingP1Param.name = "StickingProbabilityP1";
  stickingP1Param.displayName = "Sticking Probability (P1)";
  stickingP1Param.documentation =
      "Sticking probability for primary particle species";
  stickingP1Param.type = ParameterType::DOUBLE;
  stickingP1Param.defaultValue = 1.0;
  stickingP1Param.minValue = 0.0;
  stickingP1Param.maxValue = 1.0;
  stickingP1Param.category = ParameterCategory::BASIC;
  stickingP1Param.unit = "";
  stickingP1Param.stepSize = 0.01;
  stickingP1Param.required = true;
  teosProcess.parameters.push_back(stickingP1Param);

  ParameterMetadata rateP1Param;
  rateP1Param.name = "RateP1";
  rateP1Param.displayName = "Deposition Rate (P1)";
  rateP1Param.documentation = "Deposition rate for primary particle";
  rateP1Param.type = ParameterType::DOUBLE;
  rateP1Param.defaultValue = 1.0;
  rateP1Param.minValue = 0.0;
  rateP1Param.maxValue = 100.0;
  rateP1Param.category = ParameterCategory::BASIC;
  rateP1Param.unit = "";
  rateP1Param.stepSize = 0.1;
  rateP1Param.required = true;
  teosProcess.parameters.push_back(rateP1Param);

  ParameterMetadata orderP1Param;
  orderP1Param.name = "ReactionOrderP1";
  orderP1Param.displayName = "Reaction Order (P1)";
  orderP1Param.documentation = "Reaction order for primary particle";
  orderP1Param.type = ParameterType::DOUBLE;
  orderP1Param.defaultValue = 1.0;
  orderP1Param.minValue = 0.0;
  orderP1Param.maxValue = 5.0;
  orderP1Param.category = ParameterCategory::BASIC;
  orderP1Param.unit = "";
  orderP1Param.stepSize = 0.1;
  orderP1Param.required = true;
  teosProcess.parameters.push_back(orderP1Param);

  ParameterMetadata stickingP2Param;
  stickingP2Param.name = "StickingProbabilityP2";
  stickingP2Param.displayName = "Sticking Probability (P2)";
  stickingP2Param.documentation = "Sticking probability for secondary particle "
                                  "species (dual particle model only)";
  stickingP2Param.type = ParameterType::DOUBLE;
  stickingP2Param.defaultValue = 0.0;
  stickingP2Param.minValue = 0.0;
  stickingP2Param.maxValue = 1.0;
  stickingP2Param.category = ParameterCategory::BASIC;
  stickingP2Param.unit = "";
  stickingP2Param.stepSize = 0.01;
  stickingP2Param.required = false;
  teosProcess.parameters.push_back(stickingP2Param);

  ParameterMetadata rateP2Param;
  rateP2Param.name = "RateP2";
  rateP2Param.displayName = "Deposition Rate (P2)";
  rateP2Param.documentation =
      "Deposition rate for secondary particle (dual particle model only)";
  rateP2Param.type = ParameterType::DOUBLE;
  rateP2Param.defaultValue = 0.0;
  rateP2Param.minValue = 0.0;
  rateP2Param.maxValue = 100.0;
  rateP2Param.category = ParameterCategory::BASIC;
  rateP2Param.unit = "";
  rateP2Param.stepSize = 0.1;
  rateP2Param.required = false;
  teosProcess.parameters.push_back(rateP2Param);

  ParameterMetadata orderP2Param;
  orderP2Param.name = "ReactionOrderP2";
  orderP2Param.displayName = "Reaction Order (P2)";
  orderP2Param.documentation =
      "Reaction order for secondary particle (dual particle model only)";
  orderP2Param.type = ParameterType::DOUBLE;
  orderP2Param.defaultValue = 0.0;
  orderP2Param.minValue = 0.0;
  orderP2Param.maxValue = 5.0;
  orderP2Param.category = ParameterCategory::BASIC;
  orderP2Param.unit = "";
  orderP2Param.stepSize = 0.1;
  orderP2Param.required = false;
  teosProcess.parameters.push_back(orderP2Param);

  ParameterMetadata depositionMaterialParam;
  depositionMaterialParam.name = "DepositionMaterial";
  depositionMaterialParam.displayName = "Deposition Material";
    depositionMaterialParam.documentation = "Material to deposit (typically SiO2 for TEOS)";
    depositionMaterialParam.type = ParameterType::STRING;
    depositionMaterialParam.defaultValue = std::string("SiO2");
  depositionMaterialParam.category = ParameterCategory::ADVANCED;
  depositionMaterialParam.required = false;
  teosProcess.parameters.push_back(depositionMaterialParam);

  auto factory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                    vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 10.0);
    int modelType = registry.getParameter<int>(params, "ModelType", 0);
    double stickingP1 =
        registry.getParameter<double>(params, "StickingProbabilityP1", 1.0);
    double rateP1 = registry.getParameter<double>(params, "RateP1", 1.0);
    double orderP1 =
        registry.getParameter<double>(params, "ReactionOrderP1", 1.0);
    double stickingP2 =
        registry.getParameter<double>(params, "StickingProbabilityP2", 0.0);
    double rateP2 = registry.getParameter<double>(params, "RateP2", 0.0);
    double orderP2 =
        registry.getParameter<double>(params, "ReactionOrderP2", 0.0);

        std::string depositionMaterialName =
            registry.getParameter<std::string>(params, "DepositionMaterial", "SiO2");
        viennaps::Material depositionMaterial =
            ViennaPSMeta::resolveMaterialFromString(depositionMaterialName);

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;

          psDomain->duplicateTopLevelSet(depositionMaterial);

          viennaps::SmartPointer<viennaps::ProcessModelCPU<NumericType, Dim>>
              model;

          if (modelType == 0 || rateP2 == 0.0) {
            model = viennaps::SmartPointer<
                viennaps::TEOSDeposition<NumericType, Dim>>::New(stickingP1,
                                                                 rateP1,
                                                                 orderP1);

            VPSLOG_DEBUG(nullptr, "Sticking probability: ", stickingP1);
            VPSLOG_DEBUG(nullptr, "Deposition rate: ", rateP1, " nm/s");
            VPSLOG_DEBUG(nullptr, "Reaction order: ", orderP1);
          } else {
            model = viennaps::SmartPointer<
                viennaps::TEOSDeposition<NumericType, Dim>>::New(stickingP1,
                                                                 rateP1,
                                                                 orderP1,
                                                                 stickingP2,
                                                                 rateP2,
                                                                 orderP2);

            VPSLOG_DEBUG(nullptr, "Particle 1 - Sticking: ", stickingP1,
                         ", Rate: ", rateP1, " nm/s", ", Order: ", orderP1);
            VPSLOG_DEBUG(nullptr, "Particle 2 - Sticking: ", stickingP2,
                         ", Rate: ", rateP2, " nm/s", ", Order: ", orderP2);
          }

          VPSLOG_DEBUG(nullptr, "Deposition material: ",
                       viennaps::MaterialMap::toString(depositionMaterial));
          VPSLOG_DEBUG(nullptr, "Process time: ", processTime, " s");

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

  vtkViennaPSModelRegistry::getInstance().registerProcessModel(
      "TEOSDeposition", teosProcess, factory);
}

} // anonymous namespace

void ViennaPSModels::initializeDepositionModels() {
  registerTEOSDepositionProcessModel();
  registerTEOSPECVDProcessModel();
  registerSingleParticleALDProcessModel();
  registerSelectiveEpitaxyProcessModel();
}
