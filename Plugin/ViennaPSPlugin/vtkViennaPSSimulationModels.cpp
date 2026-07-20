#include "vtkViennaPSBuiltinModels.h"

#include <models/psMultiParticleProcess.hpp>
#include <models/psSingleParticleProcess.hpp>
#include <rayParticle.hpp>

#include <iostream>
#include <numeric>

namespace {
using NumericType = ViennaPSMeta::NumericType;
constexpr int D = ViennaPSMeta::D;

void registerMultiParticleProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata multiParticleProcess;
  multiParticleProcess.className = "MultiParticleProcess";
  multiParticleProcess.displayName = "Multi-Particle Process";
  multiParticleProcess.description =
      "Flexible process model supporting multiple particle types with "
      "customizable interactions";
  multiParticleProcess.type = ModelType::SIMULATION;

  ParameterMetadata numNeutralsParam;
  numNeutralsParam.name = "NumNeutralParticles";
  numNeutralsParam.displayName = "Number of Neutral Particles";
  numNeutralsParam.documentation = "Number of neutral particle types (0-5)";
  numNeutralsParam.type = ParameterType::INTEGER;
  numNeutralsParam.defaultValue = 1;
  numNeutralsParam.minValue = 0;
  numNeutralsParam.maxValue = 5;
  numNeutralsParam.category = ParameterCategory::BASIC;
  numNeutralsParam.unit = "";
  numNeutralsParam.stepSize = 1;
  numNeutralsParam.required = true;
  multiParticleProcess.parameters.push_back(numNeutralsParam);

  ParameterMetadata numIonsParam;
  numIonsParam.name = "NumIonParticles";
  numIonsParam.displayName = "Number of Ion Particles";
  numIonsParam.documentation = "Number of ion particle types (0-5)";
  numIonsParam.type = ParameterType::INTEGER;
  numIonsParam.defaultValue = 1;
  numIonsParam.minValue = 0;
  numIonsParam.maxValue = 5;
  numIonsParam.category = ParameterCategory::BASIC;
  numIonsParam.unit = "";
  numIonsParam.stepSize = 1;
  numIonsParam.required = true;
  multiParticleProcess.parameters.push_back(numIonsParam);

  for (int i = 1; i <= 5; i++) {
    std::string prefix = "Neutral" + std::to_string(i) + "_";

    ParameterMetadata stickingParam;
    stickingParam.name = prefix + "Sticking";
    stickingParam.displayName = "Neutral " + std::to_string(i) + " Sticking";
    stickingParam.documentation =
        "Sticking probability for neutral particle " + std::to_string(i);
    stickingParam.type = ParameterType::DOUBLE;
    stickingParam.defaultValue = 1.0;
    stickingParam.minValue = 0.0;
    stickingParam.maxValue = 1.0;
    stickingParam.category = ParameterCategory::BASIC;
    stickingParam.unit = "";
    stickingParam.stepSize = 0.01;
    stickingParam.required = false;
    multiParticleProcess.parameters.push_back(stickingParam);

    ParameterMetadata matStickingSiParam;
    matStickingSiParam.name = prefix + "StickingSi";
    matStickingSiParam.displayName =
        "Neutral " + std::to_string(i) + " Si Sticking";
    matStickingSiParam.documentation = "Sticking on Si for neutral particle " +
                                       std::to_string(i) +
                                       " (-1 = use default)";
    matStickingSiParam.type = ParameterType::DOUBLE;
    matStickingSiParam.defaultValue = -1.0;
    matStickingSiParam.minValue = -1.0;
    matStickingSiParam.maxValue = 1.0;
    matStickingSiParam.category = ParameterCategory::ADVANCED;
    matStickingSiParam.unit = "";
    matStickingSiParam.stepSize = 0.01;
    matStickingSiParam.required = false;
    multiParticleProcess.parameters.push_back(matStickingSiParam);
  }

  for (int i = 1; i <= 5; i++) {
    std::string prefix = "Ion" + std::to_string(i) + "_";

    ParameterMetadata sourcePowerParam;
    sourcePowerParam.name = prefix + "SourcePower";
    sourcePowerParam.displayName = "Ion " + std::to_string(i) + " Source Power";
    sourcePowerParam.documentation =
        "Angular distribution exponent for ion " + std::to_string(i);
    sourcePowerParam.type = ParameterType::DOUBLE;
    sourcePowerParam.defaultValue = 100.0;
    sourcePowerParam.minValue = 1.0;
    sourcePowerParam.maxValue = 1000.0;
    sourcePowerParam.category = ParameterCategory::BASIC;
    sourcePowerParam.unit = "";
    sourcePowerParam.stepSize = 10.0;
    sourcePowerParam.required = false;
    multiParticleProcess.parameters.push_back(sourcePowerParam);

    ParameterMetadata meanEnergyParam;
    meanEnergyParam.name = prefix + "MeanEnergy";
    meanEnergyParam.displayName = "Ion " + std::to_string(i) + " Mean Energy";
    meanEnergyParam.documentation =
        "Mean energy for ion " + std::to_string(i) + " (eV)";
    meanEnergyParam.type = ParameterType::DOUBLE;
    meanEnergyParam.defaultValue = 100.0;
    meanEnergyParam.minValue = 0.0;
    meanEnergyParam.maxValue = 1000.0;
    meanEnergyParam.category = ParameterCategory::BASIC;
    meanEnergyParam.unit = "";
    meanEnergyParam.stepSize = 10.0;
    meanEnergyParam.required = false;
    multiParticleProcess.parameters.push_back(meanEnergyParam);

    ParameterMetadata sigmaEnergyParam;
    sigmaEnergyParam.name = prefix + "SigmaEnergy";
    sigmaEnergyParam.displayName = "Ion " + std::to_string(i) + " Sigma Energy";
    sigmaEnergyParam.documentation =
        "Energy standard deviation for ion " + std::to_string(i) + " (eV)";
    sigmaEnergyParam.type = ParameterType::DOUBLE;
    sigmaEnergyParam.defaultValue = 10.0;
    sigmaEnergyParam.minValue = 0.0;
    sigmaEnergyParam.maxValue = 100.0;
    sigmaEnergyParam.category = ParameterCategory::ADVANCED;
    sigmaEnergyParam.unit = "";
    sigmaEnergyParam.stepSize = 1.0;
    sigmaEnergyParam.required = false;
    multiParticleProcess.parameters.push_back(sigmaEnergyParam);

    ParameterMetadata thresholdEnergyParam;
    thresholdEnergyParam.name = prefix + "ThresholdEnergy";
    thresholdEnergyParam.displayName =
        "Ion " + std::to_string(i) + " Threshold";
    thresholdEnergyParam.documentation =
        "Threshold energy for ion " + std::to_string(i) + " (eV)";
    thresholdEnergyParam.type = ParameterType::DOUBLE;
    thresholdEnergyParam.defaultValue = 0.0;
    thresholdEnergyParam.minValue = 0.0;
    thresholdEnergyParam.maxValue = 100.0;
    thresholdEnergyParam.category = ParameterCategory::ADVANCED;
    thresholdEnergyParam.unit = "";
    thresholdEnergyParam.stepSize = 1.0;
    thresholdEnergyParam.required = false;
    multiParticleProcess.parameters.push_back(thresholdEnergyParam);

    ParameterMetadata bSpParam;
    bSpParam.name = prefix + "B_sp";
    bSpParam.displayName = "Ion " + std::to_string(i) + " B_sp";
    bSpParam.documentation = "Angular dependency factor for ion " +
                             std::to_string(i) + " (-1 = disabled)";
    bSpParam.type = ParameterType::DOUBLE;
    bSpParam.defaultValue = -1.0;
    bSpParam.minValue = -1.0;
    bSpParam.maxValue = 10.0;
    bSpParam.category = ParameterCategory::ADVANCED;
    bSpParam.unit = "";
    bSpParam.stepSize = 0.1;
    bSpParam.required = false;
    multiParticleProcess.parameters.push_back(bSpParam);
  }

  ParameterMetadata rateFunctionParam;
  rateFunctionParam.name = "RateFunction";
  rateFunctionParam.displayName = "Rate Function Type";
  rateFunctionParam.documentation = "How to combine particle fluxes";
  rateFunctionParam.type = ParameterType::ENUM;
  rateFunctionParam.defaultValue = 0;
  rateFunctionParam.enumOptions = {"Sum", "Weighted Sum", "Product", "Custom"};
  rateFunctionParam.category = ParameterCategory::BASIC;
  rateFunctionParam.required = true;
  multiParticleProcess.parameters.push_back(rateFunctionParam);

  ParameterMetadata processTypeParam;
  processTypeParam.name = "ProcessType";
  processTypeParam.displayName = "Process Type";
  processTypeParam.documentation = "Etching or deposition process";
  processTypeParam.type = ParameterType::ENUM;
  processTypeParam.defaultValue = 0;
  processTypeParam.enumOptions = {"Etching", "Deposition"};
  processTypeParam.category = ParameterCategory::BASIC;
  processTypeParam.required = true;
  multiParticleProcess.parameters.push_back(processTypeParam);

  ParameterMetadata overallRateParam;
  overallRateParam.name = "OverallRate";
  overallRateParam.displayName = "Overall Process Rate";
  overallRateParam.documentation =
      "Overall rate multiplier (nm/s for deposition, -nm/s for etching)";
  overallRateParam.type = ParameterType::DOUBLE;
  overallRateParam.defaultValue = 1.0;
  overallRateParam.minValue = -100.0;
  overallRateParam.maxValue = 100.0;
  overallRateParam.category = ParameterCategory::BASIC;
  overallRateParam.unit = "";
  overallRateParam.stepSize = 0.1;
  overallRateParam.required = true;
  multiParticleProcess.parameters.push_back(overallRateParam);

  auto factory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                    vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 10.0);
    int numNeutrals =
        registry.getParameter<int>(params, "NumNeutralParticles", 1);
    int numIons = registry.getParameter<int>(params, "NumIonParticles", 1);
    int rateFunctionType =
        registry.getParameter<int>(params, "RateFunction", 0);
    int processType = registry.getParameter<int>(params, "ProcessType", 0);
    double overallRate =
        registry.getParameter<double>(params, "OverallRate", 1.0);

    VPSLOG_INFO(nullptr, "MultiParticleProcessModel: ",
                ViennaPSMeta::ParameterMapToString(params));

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;
          if (processType == 1) { // Deposition
            psDomain->duplicateTopLevelSet(viennaps::Material::SiO2);
          }

          auto model = viennaps::SmartPointer<
              viennaps::MultiParticleProcess<NumericType, Dim>>::New();

          for (int i = 1; i <= numNeutrals; i++) {
            std::string prefix = "Neutral" + std::to_string(i) + "_";
            double sticking =
                registry.getParameter<double>(params, prefix + "Sticking", 1.0);
            double stickingSi = registry.getParameter<double>(
                params, prefix + "StickingSi", -1.0);

            if (stickingSi >= 0) {
              std::unordered_map<viennaps::Material, NumericType>
                  materialSticking;
              materialSticking[viennaps::Material::Si] = stickingSi;
              model->addNeutralParticle(materialSticking, sticking,
                                        "neutralFlux");
            } else {
              model->addNeutralParticle(sticking, "neutralFlux");
            }

            VPSLOG_INFO(nullptr, "Added neutral particle ", i,
                        " with sticking = ", sticking);
          }

          for (int i = 1; i <= numIons; i++) {
            std::string prefix = "Ion" + std::to_string(i) + "_";
            double sourcePower = registry.getParameter<double>(
                params, prefix + "SourcePower", 100.0);
            double meanEnergy = registry.getParameter<double>(
                params, prefix + "MeanEnergy", 100.0);
            double sigmaEnergy = registry.getParameter<double>(
                params, prefix + "SigmaEnergy", 10.0);
            double thresholdEnergy = registry.getParameter<double>(
                params, prefix + "ThresholdEnergy", 0.0);
            double bSp =
                registry.getParameter<double>(params, prefix + "B_sp", -1.0);

            model->addIonParticle(sourcePower, 0.0, 90.0, 80.0, bSp, meanEnergy,
                                  sigmaEnergy, thresholdEnergy, 89.0, 10.0,
                                  "ionFlux");

            VPSLOG_INFO(nullptr, "Added ion particle ", i,
                        " with mean energy = ", meanEnergy, " eV");
          }

          double sign = (processType == 0) ? -1.0 : 1.0; // negative for etching
          switch (rateFunctionType) {
          case 0:
            model->setRateFunction(
                [overallRate, sign](const std::vector<NumericType> &fluxes,
                                    const viennaps::Material &material) {
                  return sign * overallRate *
                         std::accumulate(fluxes.begin(), fluxes.end(), 0.0);
                });
            break;
          case 1:
            model->setRateFunction([overallRate, sign, numNeutrals](
                                       const std::vector<NumericType> &fluxes,
                                       const viennaps::Material &material) {
              NumericType sum = 0;
              for (size_t i = 0; i < fluxes.size(); i++) {
                if (i < numNeutrals) {
                  sum += fluxes[i];
                } else {
                  sum += 2.0 * fluxes[i];
                }
              }
              return sign * overallRate * sum;
            });
            break;
          case 2:
            model->setRateFunction(
                [overallRate, sign](const std::vector<NumericType> &fluxes,
                                    const viennaps::Material &material) {
                  NumericType product = 1.0;
                  for (const auto &flux : fluxes) {
                    product *= flux;
                  }
                  return sign * overallRate * product;
                });
            break;
          case 3:
          default:
            model->setRateFunction(
                [overallRate, sign](const std::vector<NumericType> &fluxes,
                                    const viennaps::Material &material) {
                  return sign * overallRate *
                         std::accumulate(fluxes.begin(), fluxes.end(), 0.0);
                });
            break;
          }

          VPSLOG_DEBUG(nullptr, "Process type: ",
                       (processType == 0 ? "Etching" : "Deposition"));
          VPSLOG_DEBUG(nullptr, "Number of neutral particles: ", numNeutrals);
          VPSLOG_DEBUG(nullptr, "Number of ion particles: ", numIons);
          VPSLOG_DEBUG(nullptr, "Rate function: ", rateFunctionType);
          VPSLOG_DEBUG(nullptr, "Overall rate: ", overallRate, " nm/s");
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

  multiParticleProcess.parameters.push_back(makeNumRaysPerPointParam());

  vtkViennaPSModelRegistry::getInstance().registerProcessModel(
      "MultiParticleProcess", multiParticleProcess, factory);
}

void registerSingleParticleProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata singleParticleProcess;
  singleParticleProcess.className = "SingleParticleProcess";
  singleParticleProcess.displayName = "Single Particle Process";
  singleParticleProcess.description =
      "Single particle etching/deposition with diffuse reflections";
  singleParticleProcess.type = ModelType::SIMULATION;

  ParameterMetadata rateParam;
  rateParam.name = "Rate";
  rateParam.displayName = "Process Rate";
  rateParam.documentation = "Etch/deposition rate (nm/s). Positive for "
                            "deposition, negative for etching";
  rateParam.type = ParameterType::DOUBLE;
  rateParam.defaultValue = 1.0;
  rateParam.minValue = -100.0;
  rateParam.maxValue = 100.0;
  rateParam.category = ParameterCategory::BASIC;
  rateParam.unit = "";
  rateParam.stepSize = 0.5;
  rateParam.required = true;
  singleParticleProcess.parameters.push_back(rateParam);

  ParameterMetadata stickingParam;
  stickingParam.name = "StickingProbability";
  stickingParam.displayName = "Sticking Probability";
  stickingParam.documentation =
      "Probability of particle sticking to the surface (0-1)";
  stickingParam.type = ParameterType::DOUBLE;
  stickingParam.defaultValue = 0.1;
  stickingParam.minValue = 0.0;
  stickingParam.maxValue = 1.0;
  stickingParam.category = ParameterCategory::BASIC;
  stickingParam.unit = "";
  stickingParam.stepSize = 0.1;
  stickingParam.required = true;
  singleParticleProcess.parameters.push_back(stickingParam);

  ParameterMetadata sourceParam;
  sourceParam.name = "SourcePower";
  sourceParam.displayName = "Source Distribution Power";
  sourceParam.documentation =
      "Exponent for the angular distribution of the particle source";
  sourceParam.type = ParameterType::DOUBLE;
  sourceParam.defaultValue = 1.0;
  sourceParam.minValue = 0.0;
  sourceParam.maxValue = 10.0;
  sourceParam.category = ParameterCategory::BASIC;
  sourceParam.unit = "";
  sourceParam.stepSize = 0.5;
  sourceParam.required = true;
  singleParticleProcess.parameters.push_back(sourceParam);

  ParameterMetadata maskParam;
  maskParam.name = "MaskMaterials";
  maskParam.displayName = "Materials to mask";
  maskParam.documentation = "Select materials to be masked (zero rate)";
  maskParam.type = ParameterType::MATERIAL_LIST;
  maskParam.defaultValue = MaterialListValue{{}}; // Empty by default
  maskParam.category = ParameterCategory::BASIC;
  maskParam.required = false;

  auto materialNames = getAllMaterialNames();
  maskParam.enumOptions = materialNames;
  for (size_t i = 0; i < materialNames.size(); ++i) {
    maskParam.materialMap[static_cast<int>(i)] = materialNames[i];
  }
  singleParticleProcess.parameters.push_back(maskParam);

  ParameterMetadata depositionMaterialParam;
  depositionMaterialParam.name = "DepositionMaterial";
  depositionMaterialParam.displayName = "Deposition Material";
  depositionMaterialParam.documentation = "Material to deposit when the rate is positive (built-in name like SiO2, Si3N4, or a custom name)";
  depositionMaterialParam.type = ParameterType::STRING;
  depositionMaterialParam.defaultValue = std::string("SiO2");
  depositionMaterialParam.category = ParameterCategory::BASIC;
  depositionMaterialParam.required = false;
  depositionMaterialParam.visibilityCondition = "Rate > 0";  // only relevant for deposition
  singleParticleProcess.parameters.push_back(depositionMaterialParam);

  auto factory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                    vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 5.0);
    double rate = registry.getParameter<double>(params, "Rate", 1.0);
    double stickingProbability =
        registry.getParameter<double>(params, "StickingProbability", 1.0);
    double sourcePower =
        registry.getParameter<double>(params, "SourcePower", 1.0);

    std::vector<viennaps::Material> maskMaterials;
    if (params.find("MaskMaterials") != params.end()) {
      auto matList = registry.getParameter<MaterialListValue>(
          params, "MaskMaterials", MaterialListValue{{}});

      for (int matId : matList.materialIds) {
        auto material = static_cast<viennaps::Material>(matId);
        if (material != viennaps::Material::Undefined) {
          maskMaterials.push_back(material);
        }
      }
    }

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;

          if (rate > 0) {
            std::string depositionMaterialName =
                registry.getParameter<std::string>(params, "DepositionMaterial", "SiO2");
            psDomain->duplicateTopLevelSet(
                ViennaPSMeta::resolveMaterialFromString(depositionMaterialName));
          }

          viennaps::SmartPointer<viennaps::ProcessModelCPU<NumericType, Dim>>
              model;

          if (!maskMaterials.empty()) {
            model = viennaps::SmartPointer<viennaps::SingleParticleProcess<
                NumericType, Dim>>::New(rate, stickingProbability, sourcePower,
                                        maskMaterials);
          } else {
            model = viennaps::SmartPointer<viennaps::SingleParticleProcess<
                NumericType, Dim>>::New(rate, stickingProbability, sourcePower);
          }

          VPSLOG_DEBUG(nullptr, "ProcessTime: ", processTime);
          VPSLOG_DEBUG(nullptr, "Rate: ", rate, " nm/s");
          VPSLOG_DEBUG(nullptr, "Sticking probability: ", stickingProbability);
          VPSLOG_DEBUG(nullptr, "Source power: ", sourcePower);
          if (!maskMaterials.empty()) {
            VPSLOG_DEBUG(nullptr, "Mask materials: ", maskMaterials.size());
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

  singleParticleProcess.parameters.push_back(makeNumRaysPerPointParam());

  vtkViennaPSModelRegistry::getInstance().registerProcessModel(
      "SingleParticleProcess", singleParticleProcess, factory);
}

}

void ViennaPSModels::initializeSimulationModels() {
  registerSingleParticleProcessModel();
  registerMultiParticleProcessModel();
}
