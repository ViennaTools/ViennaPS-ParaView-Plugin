#include "vtkViennaPSBuiltinModels.h"

#include <models/psDirectionalProcess.hpp>
#include <models/psGeometricDistributionModels.hpp>
#include <models/psIsotropicProcess.hpp>

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
  sphereDistribution.type = ModelType::EMULATION;

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
  boxDistribution.type = ModelType::EMULATION;

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

void registerDirectionalProcessModel() {
  using namespace ViennaPSMeta;

  ModelMetadata directionalProcess;
  directionalProcess.className = "DirectionalProcess";
  directionalProcess.displayName = "Directional Process";
  directionalProcess.description =
      "Directional etching/deposition with visibility calculations";
  directionalProcess.type = ModelType::EMULATION;

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
  isotropicProcess.type = ModelType::EMULATION;

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

  ParameterMetadata depositionMaterialParam;
  depositionMaterialParam.name = "DepositionMaterial";
  depositionMaterialParam.displayName = "Deposition Material";
  depositionMaterialParam.documentation = "Material to deposit when the rate is positive (built-in name like SiO2, Si3N4, or a custom name)";
  depositionMaterialParam.type = ParameterType::STRING;
  depositionMaterialParam.defaultValue = std::string("SiO2");
  depositionMaterialParam.category = ParameterCategory::BASIC;
  depositionMaterialParam.required = false;
  depositionMaterialParam.visibilityCondition = "Rate > 0";  // only relevant for deposition
  isotropicProcess.parameters.push_back(depositionMaterialParam);

  auto factory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                    vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    double processTime =
        registry.getParameter<double>(params, "ProcessTime", 5.0);
    double processRate = registry.getParameter<double>(params, "Rate", 4.0);
    std::string depositionMaterialName =
        registry.getParameter<std::string>(params, "DepositionMaterial", "SiO2");

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;

          // Positive rate means deposition: add the selected material on top
          // of the domain before running the process. Etching or zero-rate
          // cases keep the existing surface and rely on mask handling below.
          if (processRate > 0) {
            psDomain->duplicateTopLevelSet(
                ViennaPSMeta::resolveMaterialFromString(depositionMaterialName));
          }

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

}

void ViennaPSModels::initializeEmulationModels() {
  registerIsotropicProcessModel();
  registerDirectionalProcessModel();
  registerSphereDistributionModel();
  registerBoxDistributionModel();
}
