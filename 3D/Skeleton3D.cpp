#include <algorithm>

#include <animation/animation.h>

#include "Animation/BlendTree/BlendTree.h"
#include "Animation/BlendTree/SkeletonBlendNodes.h"
#include "3D/Skeleton3D.h"

namespace bt = BlendTree;

namespace Animation
{
  Skeleton3D::Instance::Instance() : baseSkeleton{ nullptr }, instance{ nullptr }, blendTree{nullptr}
  {
    
  }

  Skeleton3D::Instance::~Instance()
  {
    if (blendTree) { delete blendTree; blendTree = nullptr; }
    if (instance) { delete instance; instance = nullptr; }
  }

  void Skeleton3D::Instance::initBlendTree()
  {
    if (blendTree) { delete blendTree; }

    blendTree = new bt::BlendTree();
    blendTree->startRenderContext("Simple.json");
    {
      auto blendOutputNodeInterim = new bt::SkeletonOutputNode("Output");
      blendOutputNodeInterim->setSkeletonInstance(this);
      blendOutput = bt::BlendNodeWPtr(blendTree->setOutputNode(blendOutputNodeInterim));
    }

    for (size_t i = 0; i < baseSkeleton->getAnimationCount(); ++i)
    {
      if (auto animation = baseSkeleton->getAnimation(i))
      {
        blendTree->setReference(animation->name_id(), animation);
      }
    }

    blendTree->setBindPose(&instance->bind_pose());
  }

  void Skeleton3D::Instance::setWorldTransform(const gef::Matrix44& transform)
  {
    instance->set_transform(transform);
  }

  void Skeleton3D::Instance::setPose(const gef::SkeletonPose& newPose)
  {
    instance->UpdateBoneMatrices(newPose);
  }

	gef::SkeletonPose* Skeleton3D::Instance::getPose()
	{
		if (auto outputBlend = blendOutput.lock())
		{
			if (auto a = dynamic_cast<bt::SkeletonOutputNode*>(outputBlend.get()))
			{
				return a->aaa;
			}
		}
		return nullptr;
	}

	void Skeleton3D::Instance::update(float dt)
  {
    instance->UpdateBoneMatrices(instance->bind_pose());
    if (blendTree)
    {
      blendTree->updateNodes(dt);
    }

    if (auto outputBlend = blendOutput.lock())
    {
      if (auto a = dynamic_cast<bt::SkeletonOutputNode*>(outputBlend.get()))
      {
        
        
      }
    }
    
  }

  void Skeleton3D::Instance::render(gef::Renderer3D* renderer) const
  {
    renderer->DrawSkinnedMesh(*instance, instance->bone_matrices());
  }

  Skeleton3D::Skeleton3D() : skeleton{nullptr}, mesh{nullptr}
  {
    
  }

  void Skeleton3D::bindTo(Skeleton3D::Instance& inst)
  {
    inst.baseSkeleton = this;
    if (inst.instance) { delete inst.instance; }

    // Instance
    inst.instance = new gef::SkinnedMeshInstance(*skeleton);
    inst.instance->set_mesh(mesh);
    {
      gef::Matrix44 identity;
      identity.SetIdentity();
      inst.instance->set_transform(identity);
    }
    inst.initBlendTree();
  }

  UInt Skeleton3D::addAnimation(gef::StringId labelID, const gef::Animation* animation)
  {
    return animations.add(labelID, animation).getHeapID();
  }

  UInt Skeleton3D::getAnimationID(Label label) const
  {
    return animations.getID(label);
  }

  UInt Skeleton3D::getAnimationID(gef::StringId labelID) const
  {
    return animations.getID(labelID);
  }

  Skeleton3D::IKController::IKController() : unreachableTolerance{ 7.5f }, reachTolerance{ 0.001f }, maxIterations{ 25 }, effectorLength{ 1.f }
  {

  }

  void Skeleton3D::IKController::bind(const Skeleton3D* skeleton, gef::StringId effectorJoint, gef::StringId rootJoint, size_t maxDepth)
  {
    boneIndices.clear();

    auto bones = skeleton->getSkeleton();

    // Get the end, then cache indices along the chain
    Int32 jointIdx = bones->FindJointIndex(effectorJoint);
    while (jointIdx >= 0 && boneIndices.size() < maxDepth)
    {
      boneIndices.push_back(UInt(jointIdx));
      
      // Setup for the next bone
      auto joint = bones->joints()[jointIdx];
      if (joint.name_id == rootJoint) { break; } // If this is the root, exit
      jointIdx = joint.parent;
    }

    // Reverse the result to flow from the root
    std::reverse(boneIndices.begin(), boneIndices.end());
  }

  UInt Skeleton3D::IKController::resolveFABRIK(const gef::Transform& target, const gef::SkeletonPose& bindPose, gef::SkeletonPose& pose, const gef::SkinnedMeshInstance& animatedModel)
  {
		std::vector<gef::Vector4> jointPosition;
		std::vector<float> jointLength;

		// Joint positions and bone length with the additional target on the leaf bone
		jointPosition.resize(boneIndices.size() + 1);
		jointLength.resize(boneIndices.size() + 1);
		float totalLength = .0f;

		{
			// Start with the location off of the leaf bone (just the joint for now)
			auto previousBonePos = jointPosition.back() = bindPose.global_pose()[boneIndices.back()].GetTranslation();
			jointLength.back() = .0f;

			for (int i = boneIndices.size() - 1; i >= 0; --i)
			{
				Int32 boneID = boneIndices[i];

				auto& pos = jointPosition[i] = bindPose.global_pose()[boneID].GetTranslation();
				auto& length = jointLength[i] = (pos - previousBonePos).Length();

				totalLength += length;
				previousBonePos = pos;
			}
		}
		totalLength += jointLength[boneIndices.size() - 1] = effectorLength;

		gef::Vector4 destPos;
		{
			gef::Matrix44 worldToModelTransform;
			worldToModelTransform.Inverse(animatedModel.transform());

			destPos = target.translation().Transform(worldToModelTransform);
		}

		int iterationCount = 0;
		float reachDistance = (destPos - jointPosition[0]).Length();
		bool unreachable = reachDistance >= totalLength + unreachableTolerance;

		if (unreachable)
		{
			// Not reachable, means fully extend!
			for (int i = 0; i < boneIndices.size(); ++i)
			{
				const auto& pos = jointPosition[i];
				reachDistance = (destPos - pos).Length();

				// Perform a simple lerp, extending as far as possible
				float linearParam = jointLength[i] / reachDistance;

				auto& childPos = jointPosition[i + 1];
				childPos.Lerp(pos, destPos, linearParam);
			}
			++iterationCount;
		}
		else
		{
			// Reachable, means do the main FABRIK algorithm

			auto originalRootPos = jointPosition[0]; // Cache an anchor of the root position

			// Initial check if the end effector is on the target (square distance)
			reachDistance = (destPos - jointPosition.back()).LengthSqr();
			while ((reachDistance > reachTolerance) && (iterationCount < maxIterations))
			{
				// FORWARD REACHING STAGE //
				// We want to overextend the end effector and smoothly reconstruct everything towards it
				jointPosition.back() = destPos; // Snap to the destination
				for (int i = boneIndices.size() - 1; i > 0; --i)
				{
					auto& pos = jointPosition[i];
					const auto& childPos = jointPosition[i + 1];

					reachDistance = (childPos - pos).Length();

					// Lerp towards the requested child position up to the accessible length
					float linearParam = jointLength[i] / reachDistance;

					pos.Lerp(childPos, pos, linearParam);
				}

				// BACKWARDS REACHING STAGE //
				// Now snap the root back and apply some push back
				jointPosition[0] = originalRootPos; // Snap back!
				for (int i = 0; i < boneIndices.size(); ++i)
				{
					const auto& pos = jointPosition[i];
					auto& childPos = jointPosition[i + 1];

					reachDistance = (childPos - pos).Length();

					// Lerp the child towards the requested anchored position up to the accessible length
					float linearParam = jointLength[i] / reachDistance;

					childPos.Lerp(pos, childPos, linearParam);
				}

				// PREPARE NEXT PASS //
				// Check again if the end effector is on the target (square distance)
				reachDistance = (destPos - jointPosition.back()).LengthSqr();
				++iterationCount;
			}
		}

		// Compute global transforms and apply
		{
			// Hold the transform of the previous global transform
			gef::Matrix44 priorTransform;
			priorTransform.SetIdentity();

			// Set to the parent transform of the IK 'root' if feasible
			{
				const gef::Joint& joint = pose.skeleton()->joint(boneIndices[0]);
				if (joint.parent >= 0)
				{
					priorTransform = pose.global_pose()[joint.parent];
				}
			}

			for (int i = 0; i < boneIndices.size(); ++i)
			{
				Int32 boneID = boneIndices[i];
				const auto& pos = jointPosition[i];
				auto& childPos = jointPosition[i + 1];

				gef::Vector4 forward = pos - childPos;
				forward.Normalise();

				const gef::Matrix44& previousTransform = bindPose.global_pose()[boneID];
				gef::Vector4 oldRight = previousTransform.GetRow(1);

				gef::Vector4 newUp = forward.CrossProduct(oldRight);
				gef::Vector4 newRight = newUp.CrossProduct(forward);

				gef::Matrix44 newTransform;
				newTransform.SetIdentity();
				newTransform.SetTranslation(pos);
				newTransform.SetRow(0, forward);
				newTransform.set_m(0, 3, 0);
				newTransform.SetRow(1, newRight);
				newTransform.set_m(1, 3, 0);
				newTransform.SetRow(2, newUp);
				newTransform.set_m(2, 3, 0);
				newTransform.NormaliseRotation();

				// Compute local transform from the parent's inverse
				{
					gef::Matrix44 parentInv;
					parentInv.Inverse(priorTransform);
					pose.local_pose()[boneID] = newTransform * parentInv;

					priorTransform = newTransform;
				}
			}
		}

		// Forward kinematics pass
		pose.CalculateGlobalPose();

		return iterationCount;
  }

}
