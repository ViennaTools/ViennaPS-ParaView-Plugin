#include "vtkViennaPSBuiltinModels.h"

#include <models/psDirectionalProcess.hpp>
#include <models/psGeometricDistributionModels.hpp>
#include <models/psIsotropicProcess.hpp>
#include <models/psMultiParticleProcess.hpp>
#include <models/psSingleParticleProcess.hpp>
#include <rayParticle.hpp>

#include <iostream>
#include <numeric>

namespace {
using NumericType = ViennaPSMeta::NumericType;
constexpr int D = ViennaPSMeta::D;

void registerSphereDistributionModel() {
  using namespace ViennaPSMeta;

  ModelMetadata sphereDistribution;
  sphereDistribution.className = "SphereDistribution";
  sphereDistribution.displayName = "Sphere Distribution";
  sphereDistribution.description =
      "Deposits or etches a spherical shape on the surface";
  sphereDistribution.type = ModelType::PROCESS;

  ParameterMetadata radiusParam;
  radiusParam.name = "Radius";
  radiusParam.displayName = "Sphere Radius";
  radiusParam.documentation =
      "Radius of the sphere to be deposited/etched (nm)";
  radiusParam.type = ParameterType::DOUBLE;
  radiusParam.defaultValue = 10.0;
  radiusParam.minValue = 0.1;
  radiusParam.maxValue = 1000.0;
  radiusParam.category = ParameterCategory::BASIC;
  radiusParam.unit = "";
  radiusParam.stepSize = 0.1;
  radiusParam.required = true;
  sphereDistribution.parameters.push_back(radiusParam);

  ParameterMetadata gridDeltaParam;
  gridDeltaParam.name = "GridDelta";
  gridDeltaParam.displayName = "Grid Resolution";
  gridDeltaParam.documentation =
      "Grid spacing for the level set representation (nm)";
  gridDeltaParam.type = ParameterType::DOUBLE;
  gridDeltaParam.defaultValue = 0.5;
  gridDeltaParam.minValue = 0.01;
  gridDeltaParam.maxValue = 10.0;
  gridDeltaParam.category = ParameterCategory::ADVANCED;
  gridDeltaParam.unit = "";
  gridDeltaParam.stepSize = 0.01;
  gridDeltaParam.required = true;
  sphereDistribution.parameters.push_back(gridDeltaParam);

  ParameterMetadata maskParam;
  maskParam.name = "MaskMaterials";
  maskParam.displayName = "Materials to mask";
  maskParam.documentation =
      "Select materials where the distribution should not be applied";
  maskParam.type = ParameterType::MATERIAL_LIST;
  maskParam.defaultValue = MaterialListValue{{}};
  maskParam.category = ParameterCategory::ADVANCED;
  maskParam.required = false;
  {
    auto materialNames = getAllMaterialNames();
    maskParam.enumOptions = materialNames;
    for (size_t i = 0; i < materialNames.size(); ++i) {
      maskParam.materialMap[static_cast<int>(i)] = materialNames[i];
    }
  }
  sphereDistribution.parameters.push_back(maskParam);

  ParameterMetadata processTimeParam;
  processTimeParam.name = "ProcessTime";
  processTimeParam.displayName = "Process Time";
  processTimeParam.documentation = "Duration of the process (s)";
  processTimeParam.type = ParameterType::DOUBLE;
  processTimeParam.defaultValue = 1.0;
  processTimeParam.minValue = 0.001;
  processTimeParam.maxValue = 1000.0;
  processTimeParam.category = ParameterCategory::BASIC;
  processTimeParam.unit = "";
  processTimeParam.stepSize = 0.1;
  processTimeParam.required = true;
  sphereDistribution.parameters.push_back(processTimeParam);

  auto sphereDistModel = [](std::shared_ptr<void> psDomainVoid, int dimension,
                            vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    double radius = registry.getParameter<double>(params, "Radius", 10.0);
    double gridDelta = registry.getParameter<double>(params, "GridDelta", 0.5);
    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 1.0);

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

    VPSLOG_INFO(nullptr, "sphereDistModel: ",
                ViennaPSMeta::ParameterMapToString(params));

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;

          auto model = viennaps::SmartPointer<
              viennaps::SphereDistribution<NumericType, Dim>>::New(radius);
          for (const auto &mat : maskMaterials) {
            model->addMaskMaterial(mat);
          }

          viennaps::Process<NumericType, Dim> process;
          process.setDomain(psDomain);
          process.setProcessModel(model);
          process.setProcessDuration(processTime);

#ifndef MULTI_STEP
          process.apply();
#else
            const int numSteps = 10;
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
      "SphereDistribution", sphereDistribution, sphereDistModel);
}

void registerBoxDistributionModel() {
  using namespace ViennaPSMeta;

  ModelMetadata boxDistribution;
  boxDistribution.className = "BoxDistribution";
  boxDistribution.displayName = "Box Distribution";
  boxDistribution.description =
      "Deposits or etches a box/rectangular shape on the surface";
  boxDistribution.type = ModelType::PROCESS;

  ParameterMetadata halfAxisXParam;
  halfAxisXParam.name = "HalfAxisX";
  halfAxisXParam.displayName = "Half Width (X)";
  halfAxisXParam.documentation = "Half-width of the box in X direction (nm)";
  halfAxisXParam.type = ParameterType::DOUBLE;
  halfAxisXParam.defaultValue = 10.0;
  halfAxisXParam.minValue = 0.1;
  halfAxisXParam.maxValue = 1000.0;
  halfAxisXParam.category = ParameterCategory::BASIC;
  halfAxisXParam.unit = "";
  halfAxisXParam.stepSize = 0.1;
  halfAxisXParam.required = true;
  boxDistribution.parameters.push_back(halfAxisXParam);

  ParameterMetadata halfAxisYParam;
  halfAxisYParam.name = "HalfAxisY";
  halfAxisYParam.displayName = "Half Width (Y)";
  halfAxisYParam.documentation = "Half-width of the box in Y direction (nm)";
  halfAxisYParam.type = ParameterType::DOUBLE;
  halfAxisYParam.defaultValue = 10.0;
  halfAxisYParam.minValue = 0.1;
  halfAxisYParam.maxValue = 1000.0;
  halfAxisYParam.category = ParameterCategory::BASIC;
  halfAxisYParam.unit = "";
  halfAxisYParam.stepSize = 0.1;
  halfAxisYParam.required = true;
  boxDistribution.parameters.push_back(halfAxisYParam);

  ParameterMetadata halfAxisZParam;
  halfAxisZParam.name = "HalfAxisZ";
  halfAxisZParam.displayName = "Half Height (Z)";
  halfAxisZParam.documentation = "Half-height of the box in Z direction (nm)";
  halfAxisZParam.type = ParameterType::DOUBLE;
  halfAxisZParam.defaultValue = 10.0;
  halfAxisZParam.minValue = 0.1;
  halfAxisZParam.maxValue = 1000.0;
  halfAxisZParam.category = ParameterCategory::BASIC;
  halfAxisZParam.unit = "";
  halfAxisZParam.stepSize = 0.1;
  halfAxisZParam.required = true;
  boxDistribution.parameters.push_back(halfAxisZParam);

  ParameterMetadata gridDeltaParam;
  gridDeltaParam.name = "GridDelta";
  gridDeltaParam.displayName = "Grid Resolution";
  gridDeltaParam.documentation =
      "Grid spacing for the level set representation (nm)";
  gridDeltaParam.type = ParameterType::DOUBLE;
  gridDeltaParam.defaultValue = 0.5;
  gridDeltaParam.minValue = 0.01;
  gridDeltaParam.maxValue = 10.0;
  gridDeltaParam.category = ParameterCategory::ADVANCED;
  gridDeltaParam.unit = "";
  gridDeltaParam.stepSize = 0.01;
  gridDeltaParam.required = true;
  boxDistribution.parameters.push_back(gridDeltaParam);

  ParameterMetadata maskParam;
  maskParam.name = "MaskMaterials";
  maskParam.displayName = "Materials to mask";
  maskParam.documentation =
      "Select materials where the distribution should not be applied";
  maskParam.type = ParameterType::MATERIAL_LIST;
  maskParam.defaultValue = MaterialListValue{{}};
  maskParam.category = ParameterCategory::ADVANCED;
  maskParam.required = false;
  {
    auto materialNames = getAllMaterialNames();
    maskParam.enumOptions = materialNames;
    for (size_t i = 0; i < materialNames.size(); ++i) {
      maskParam.materialMap[static_cast<int>(i)] = materialNames[i];
    }
  }
  boxDistribution.parameters.push_back(maskParam);

  ParameterMetadata processTimeParam;
  processTimeParam.name = "ProcessTime";
  processTimeParam.displayName = "Process Time";
  processTimeParam.documentation = "Duration of the process (s)";
  processTimeParam.type = ParameterType::DOUBLE;
  processTimeParam.defaultValue = 1.0;
  processTimeParam.minValue = 0.001;
  processTimeParam.maxValue = 1000.0;
  processTimeParam.category = ParameterCategory::BASIC;
  processTimeParam.unit = "";
  processTimeParam.stepSize = 0.1;
  processTimeParam.required = true;
  boxDistribution.parameters.push_back(processTimeParam);

  auto boxDistModel = [](std::shared_ptr<void> psDomainVoid, int dimension,
                         vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    double halfAxisX = registry.getParameter<double>(params, "HalfAxisX", 10.0);
    double halfAxisY = registry.getParameter<double>(params, "HalfAxisY", 10.0);
    double halfAxisZ = registry.getParameter<double>(params, "HalfAxisZ", 10.0);
    double gridDelta = registry.getParameter<double>(params, "GridDelta", 0.5);
    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 1.0);

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

    VPSLOG_INFO(nullptr,
                "boxDistModel: ", ViennaPSMeta::ParameterMapToString(params));

    VPSLOG_DEBUG(nullptr, "\n=== Box Distribution Process ===");

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;

          // Create half-axes array (BoxDistribution expects 3D array even for
          // 2D)
          std::array<NumericType, 3> halfAxes = {
              static_cast<NumericType>(halfAxisX),
              static_cast<NumericType>(halfAxisY),
              (Dim == 3) ? static_cast<NumericType>(halfAxisZ) : 0.0};

          auto model = viennaps::SmartPointer<
              viennaps::BoxDistribution<NumericType, Dim>>::New(halfAxes);
          for (const auto &mat : maskMaterials) {
            model->addMaskMaterial(mat);
          }

          viennaps::Process<NumericType, Dim> process;
          process.setDomain(psDomain);
          process.setProcessModel(model);
          process.setProcessDuration(processTime);

#ifndef MULTI_STEP
          process.apply();
#else
            const int numSteps = 10;
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
      "BoxDistribution", boxDistribution, boxDistModel);
}

void registerMultiParticleProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata multiParticleProcess;
  multiParticleProcess.className = "MultiParticleProcess";
  multiParticleProcess.displayName = "Multi-Particle Process";
  multiParticleProcess.description =
      "Flexible process model supporting multiple particle types with "
      "customizable interactions";
  multiParticleProcess.type = ModelType::PROCESS;

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
      "MultiParticleProcess", multiParticleProcess, factory);
}

void registerSingleParticleProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata singleParticleProcess;
  singleParticleProcess.className = "SingleParticleProcess";
  singleParticleProcess.displayName = "Single Particle Process";
  singleParticleProcess.description =
      "Single particle etching/deposition with diffuse reflections";
  singleParticleProcess.type = ModelType::PROCESS;

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
  depositionMaterialParam.documentation =
      "Material to deposit when rate is positive";
  depositionMaterialParam.type = ParameterType::ENUM;
  depositionMaterialParam.defaultValue =
      static_cast<int>(viennaps::Material::SiO2);
  depositionMaterialParam.category = ParameterCategory::BASIC;
  depositionMaterialParam.required = false;

  depositionMaterialParam.enumOptions = materialNames;
  for (size_t i = 0; i < materialNames.size(); ++i) {
    depositionMaterialParam.materialMap[static_cast<int>(i)] = materialNames[i];
  }
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
            viennaps::Material depositionMaterial = viennaps::Material::SiO2;

            if (params.find("DepositionMaterial") != params.end()) {
              int matId = registry.getParameter<int>(
                  params, "DepositionMaterial",
                  static_cast<int>(viennaps::Material::SiO2));
              depositionMaterial = static_cast<viennaps::Material>(matId);
            }

            psDomain->duplicateTopLevelSet(depositionMaterial);
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
      "SingleParticleProcess", singleParticleProcess, factory);
}

void registerDirectionalProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata directionalProcess;
  directionalProcess.className = "DirectionalProcess";
  directionalProcess.displayName = "Directional Process";
  directionalProcess.description =
      "Directional etching/deposition with visibility calculations";
  directionalProcess.type = ModelType::PROCESS;

  ParameterMetadata dirXParam;
  dirXParam.name = "DirectionX";
  dirXParam.displayName = "Direction X";
  dirXParam.documentation = "X component of the direction vector";
  dirXParam.type = ParameterType::DOUBLE;
  dirXParam.defaultValue = 1.0;
  dirXParam.minValue = -1.5;
  dirXParam.maxValue = 1.5;
  dirXParam.category = ParameterCategory::BASIC;
  dirXParam.unit = "";
  dirXParam.stepSize = 0.1;
  dirXParam.required = true;
  directionalProcess.parameters.push_back(dirXParam);

  ParameterMetadata dirYParam;
  dirYParam.name = "DirectionY";
  dirYParam.displayName = "Direction Y";
  dirYParam.documentation = "Y component of the direction vector";
  dirYParam.type = ParameterType::DOUBLE;
  dirYParam.defaultValue = 1.0;
  dirYParam.minValue = -1.5;
  dirYParam.maxValue = 1.5;
  dirYParam.category = ParameterCategory::BASIC;
  dirYParam.unit = "";
  dirYParam.stepSize = 0.1;
  dirYParam.required = true;
  directionalProcess.parameters.push_back(dirYParam);

  ParameterMetadata dirZParam;
  dirZParam.name = "DirectionZ";
  dirZParam.displayName = "Direction Z";
  dirZParam.documentation = "Z component of the direction vector";
  dirZParam.type = ParameterType::DOUBLE;
  dirZParam.defaultValue = -1.0;
  dirZParam.minValue = -1.0;
  dirZParam.maxValue = 1.0;
  dirZParam.category = ParameterCategory::BASIC;
  dirZParam.unit = "";
  dirZParam.stepSize = 0.1;
  dirZParam.required = true;
  directionalProcess.parameters.push_back(dirZParam);

  ParameterMetadata dirVelParam;
  dirVelParam.name = "DirectionalVelocity";
  dirVelParam.displayName = "Directional Velocity";
  dirVelParam.documentation = "Directional etch/deposition rate (nm/s)";
  dirVelParam.type = ParameterType::DOUBLE;
  dirVelParam.defaultValue = 5.0;
  dirVelParam.minValue = -50.0;
  dirVelParam.maxValue = 50.0;
  dirVelParam.category = ParameterCategory::BASIC;
  dirVelParam.unit = "";
  dirVelParam.stepSize = 0.5;
  dirVelParam.required = true;
  directionalProcess.parameters.push_back(dirVelParam);

  ParameterMetadata isoVelParam;
  isoVelParam.name = "IsotropicVelocity";
  isoVelParam.displayName = "Isotropic Velocity";
  isoVelParam.documentation = "Isotropic etch/deposition rate (nm/s)";
  isoVelParam.type = ParameterType::DOUBLE;
  isoVelParam.defaultValue = 0.0;
  isoVelParam.minValue = -50.0;
  isoVelParam.maxValue = 50.0;
  isoVelParam.category = ParameterCategory::BASIC;
  isoVelParam.unit = "";
  isoVelParam.stepSize = 0.5;
  isoVelParam.required = false;
  directionalProcess.parameters.push_back(isoVelParam);

  ParameterMetadata maskParam;
  maskParam.name = "MaskMaterials";
  maskParam.displayName = "Materials to mask";
  maskParam.documentation = "Select mask materials";
  maskParam.type = ParameterType::MATERIAL_LIST;
  maskParam.defaultValue = MaterialListValue{{1}}; // Default to Mask material
  maskParam.category = ParameterCategory::BASIC;
  maskParam.required = true;

  auto materialNames = getAllMaterialNames();
  maskParam.enumOptions = materialNames;
  for (size_t i = 0; i < materialNames.size(); ++i) {
    maskParam.materialMap[static_cast<int>(i)] = materialNames[i];
  }
  directionalProcess.parameters.push_back(maskParam);

  ParameterMetadata visibilityParam;
  visibilityParam.name = "CalculateVisibility";
  visibilityParam.displayName = "Calculate Visibility";
  visibilityParam.documentation =
      "Enable visibility calculations for shadowing effects";
  visibilityParam.type = ParameterType::BOOLEAN;
  visibilityParam.defaultValue = true;
  visibilityParam.category = ParameterCategory::ADVANCED;
  visibilityParam.required = false;
  directionalProcess.parameters.push_back(visibilityParam);

  auto factory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                    vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 5.0);
    VPSLOG_DEBUG(nullptr, "Process time: ", processTime);
    double dirX = registry.getParameter<double>(params, "DirectionX", 0.0);
    double dirY = registry.getParameter<double>(params, "DirectionY", 0.0);
    double dirZ = registry.getParameter<double>(params, "DirectionZ", -1.0);
    viennaps::Vec3D<NumericType> direction{dirX, dirY, dirZ};

    double directionalVelocity =
        registry.getParameter<double>(params, "DirectionalVelocity", 5.0);
    double isotropicVelocity =
        registry.getParameter<double>(params, "IsotropicVelocity", 0.0);

    bool calculateVisibility =
        registry.getParameter<bool>(params, "CalculateVisibility", true);

    std::vector<viennaps::Material> maskMaterials;
    if (params.find("MaskMaterials") != params.end()) {
      const auto &matList =
          std::get<MaterialListValue>(params.at("MaskMaterials"));

      for (int matId : matList.materialIds) {
        auto material = static_cast<viennaps::Material>(matId);
        if (material != viennaps::Material::Undefined) {
          maskMaterials.push_back(material);
        }
      }
    }

    if (maskMaterials.empty()) {
      maskMaterials.push_back(viennaps::Material::Mask);
    }

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;

          viennaps::SmartPointer<viennaps::ProcessModelCPU<NumericType, Dim>>
              model;

          model = viennaps::
              SmartPointer<viennaps::DirectionalProcess<NumericType, Dim>>::New(
                  direction, directionalVelocity, isotropicVelocity,
                  maskMaterials, calculateVisibility);

          viennaps::Process<NumericType, Dim> process;
          process.setDomain(psDomain);
          process.setProcessModel(model);

#ifndef MULTI_STEP
          process.setProcessDuration(processTime);
          process.apply();
#else
        const int numSteps = 10;
        const double stepTime = processTime / numSteps;

        for (int step = 0; step < numSteps; ++step) {
            process.setProcessDuration(stepTime);
            process.apply();
        }
#endif

          ViennaPSModels::convertToVTK<Dim>(psDomain, output, params);
        });
  };

  vtkViennaPSModelRegistry::getInstance().registerProcessModel(
      "DirectionalProcess", directionalProcess, factory);
}

void registerIsotropicProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata isotropicProcess;
  isotropicProcess.className = "IsotropicProcess";
  isotropicProcess.displayName = "Isotropic Process";
  isotropicProcess.type = ModelType::PROCESS;

  ParameterMetadata rateParam;
  rateParam.name = "Rate";
  rateParam.displayName = "Process Rate";
  rateParam.documentation = "Isotropic etch/deposition rate (nm/s)";
  rateParam.type = ParameterType::DOUBLE;
  rateParam.defaultValue = 4.0;
  rateParam.minValue = -50.0;
  rateParam.maxValue = 50.0;
  rateParam.category = ParameterCategory::BASIC;
  rateParam.unit = "";
  rateParam.stepSize = 0.5;
  rateParam.required = true;

  isotropicProcess.parameters.push_back(rateParam);

  ParameterMetadata maskParam;
  maskParam.name = "MaskMaterials";
  maskParam.displayName = "Materials to mask";
  maskParam.documentation = "Select mask materials";
  maskParam.type = ParameterType::MATERIAL_LIST;
  maskParam.defaultValue = MaterialListValue{{1}};
  maskParam.category = ParameterCategory::BASIC;
  maskParam.required = true;

  auto materialNames = getAllMaterialNames();
  maskParam.enumOptions = materialNames;
  for (size_t i = 0; i < materialNames.size(); ++i) {
    maskParam.materialMap[static_cast<int>(i)] = materialNames[i];
  }

  isotropicProcess.parameters.push_back(maskParam);

  auto factory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                    vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 5.0);
    double processRate =
        registry.getParameter<double>(params, "ProcessRate", 1.0);

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;

          std::vector<viennaps::Material> maskMaterials;
          if (params.find("MaskMaterials") != params.end()) {
            const auto &matList =
                std::get<MaterialListValue>(params.at("MaskMaterials"));

            for (int matId : matList.materialIds) {
              auto material = static_cast<viennaps::Material>(matId);
              if (material != viennaps::Material::Undefined) {
                maskMaterials.push_back(material);
              }
            }
          }

          viennaps::SmartPointer<viennaps::ProcessModelCPU<NumericType, Dim>>
              model;

          if (maskMaterials.empty()) {
            model = viennaps::
                SmartPointer<viennaps::IsotropicProcess<NumericType, Dim>>::New(
                    processRate, viennaps::Material::Undefined);
          } else {
            model = viennaps::
                SmartPointer<viennaps::IsotropicProcess<NumericType, Dim>>::New(
                    processRate, maskMaterials);
          }

          viennaps::Process<NumericType, Dim> process;
          process.setDomain(psDomain);
          process.setProcessModel(model);
          process.setProcessDuration(processTime);

          process.apply();

          ViennaPSModels::convertToVTK<Dim>(psDomain, output, params);
        });
  };

  vtkViennaPSModelRegistry::getInstance().registerProcessModel(
      "IsotropicProcess", isotropicProcess, factory);
}

} // anonymous namespace

void ViennaPSModels::initializeDistributionModels() {
  registerIsotropicProcessModel();
  registerDirectionalProcessModel();
  registerSingleParticleProcessModel();
  registerMultiParticleProcessModel();
  registerSphereDistributionModel();
  registerBoxDistributionModel();
}
