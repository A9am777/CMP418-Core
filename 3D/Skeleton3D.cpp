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

    blendTree->setInstanceContext(this);
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

	Skeleton3D::IKController::IKController() : unreachableTolerance{ 7.5f }, reachTolerance{ 0.001f }, maxIterations{ 25 }, effectorLength{ .0f }, rightHanded{ true }, skeletonInstance{ nullptr }, staticTotalLength{.0f}
  {

  }

	void Skeleton3D::IKController::setInstance(const Skeleton3D::Instance* newInst)
	{
		skeletonInstance = newInst;

		boneIndices.clear();
		jointLength.clear();
		jointLocal.clear();
		effectorLength = .0f;
		staticTotalLength = .0f;
	}

	void Skeleton3D::IKController::setEffector(const gef::Vector4& local)
	{
		if (!skeletonInstance) { return; }

		if (boneIndices.empty())
		{
			jointLocal.push_back(local);
			return;
		}
		jointLocal.back() = local;

		// Resolve the bone length
		auto bindPose = skeletonInstance->getBindPose();
		auto& endJointGlobal = bindPose->global_pose()[boneIndices.back()];
		auto effectorPos = local.Transform(endJointGlobal);
		jointLength[boneIndices.size() - 1] = effectorLength = (effectorPos - endJointGlobal.GetTranslation()).Length();
	}

  void Skeleton3D::IKController::bind(gef::StringId effectorJoint, gef::StringId rootJoint, size_t maxDepth)
  {
		if (!skeletonInstance) { return; }

		gef::Vector4 priorEndEffectorLocal = jointLocal.empty() ? gef::Vector4::kZero : jointLocal.back();
    boneIndices.clear();
		jointLength.clear();
		jointLocal.clear();

    auto bones = skeletonInstance->getSkeleton()->getSkeleton();

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
		if (boneIndices.empty()) { return; }

    // Reverse the result to flow from the root
    std::reverse(boneIndices.begin(), boneIndices.end());

		auto bindPose = skeletonInstance->getBindPose();

		// Precompute bone lengths and cache local positions
		staticTotalLength = .0f;
		jointLength.resize(boneIndices.size() + 1); // Include the additional end effector on the leaf bone
		jointLocal.resize(jointLength.size());
		{
			auto previousBonePos = bindPose->global_pose()[boneIndices.back()].GetTranslation();
			for (int i = boneIndices.size() - 1; i >= 0; --i)
			{
				int boneID = boneIndices[i];

				jointLocal[i]  = bindPose->local_pose()[boneID].translation();

				auto& pos = bindPose->global_pose()[boneID].GetTranslation();
				auto& length = jointLength[i] = (pos - previousBonePos).Length();

				staticTotalLength += length;
				previousBonePos = pos;
			}
			jointLength.back() = .0f;
		}

		// Reset the effector
		setEffector(priorEndEffectorLocal);
  }

  UInt Skeleton3D::IKController::resolveFABRIK(const gef::Transform& target, gef::SkeletonPose& pose)
  {
		if (!skeletonInstance || boneIndices.empty()) { return SNULL; }

		// Joint positions with the additional end effector target on the leaf bone
		std::vector<gef::Vector4> jointPosition;
		jointPosition.resize(jointLength.size());

		float totalLength = staticTotalLength;
		totalLength += jointLength[boneIndices.size() - 1] = effectorLength; // Now include the effector offset

		{
			// Start with the location off of the leaf bone (just the joint for now)
			auto previousBonePos = jointPosition.back() = pose.global_pose()[boneIndices.back()].GetTranslation();
			totalLength += jointLength[boneIndices.size() - 1] = effectorLength;

			for (int i = boneIndices.size() - 1; i >= 0; --i)
			{
				int boneID = boneIndices[i];

				jointPosition[i] = pose.global_pose()[boneID].GetTranslation();
			}
		}

		// Transform the target position into model space
		gef::Vector4 destPos;
		{
			gef::Matrix44 worldToModelTransform;
			worldToModelTransform.Inverse(skeletonInstance->instance->transform());

			destPos = target.translation().Transform(worldToModelTransform);
		}

		// Set up the first pass
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

			// Lets set each joint transform then!
			for (int i = 0; i < boneIndices.size(); ++i)
			{
				Int32 boneID = boneIndices[i];
				const auto& pos = jointPosition[i];
				auto& childPos = jointPosition[i + 1];

				// Get the prior coordinate space (for twisting)
				const gef::Matrix44& previousTransform = pose.global_pose()[boneID];
				gef::Vector4 oldRight = rightHanded ? previousTransform.GetRow(1) : previousTransform.GetRow(0);

				// Orient from the parent towards the child...
				gef::Matrix44 newTransform;
				setOrientationTowards(newTransform, pos, childPos, oldRight, rightHanded);
				
				// Oops, does not account for the child's local offset
				// Shift the target and try again
				{
					auto& childLocal = jointLocal[i + 1];
					childLocal.set_y(rightHanded && childLocal.x() > 0 ? childLocal.y() : -childLocal.y());
					childLocal.set_z(rightHanded ? -childLocal.z() : childLocal.z());

					gef::Vector4 alteredTarget = childLocal.Transform(newTransform);

					// Keep within the resolved coordinate system
					oldRight = rightHanded ? newTransform.GetRow(1) : newTransform.GetRow(0);

					// For real this time
					setOrientationTowards(newTransform, pos, alteredTarget, oldRight, rightHanded);
				}

				// Finally, compute local transform from the parent's inverse
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

	void Skeleton3D::IKController::setOrientation(gef::Matrix44& mat, const gef::Vector4& forward, const gef::Vector4& right, const gef::Vector4& up)
	{
		mat.SetRow(0, forward);
		mat.set_m(0, 3, 0);
		mat.SetRow(1, right);
		mat.set_m(1, 3, 0);
		mat.SetRow(2, up);
		mat.set_m(2, 3, 0);
		mat.NormaliseRotation();
	}

	void Skeleton3D::IKController::setOrientationTowards(gef::Matrix44& mat, const gef::Vector4& location, const gef::Vector4& target, gef::Vector4 biasRight, bool rightHanded)
	{
		biasRight.Normalise();

		// Compose basis
		gef::Vector4 forward = target - location;
		forward = -forward;
		forward.Normalise();

		gef::Vector4 newUp;
		gef::Vector4 newRight;

		newUp = forward.CrossProduct(biasRight);
		newUp.Normalise();
		newRight = newUp.CrossProduct(forward);
		newRight.Normalise();

		// Combine orientation and translation
		if (rightHanded)
		{
			setOrientation(mat, forward, newRight, newUp);
		}
		else
		{
			setOrientation(mat, newRight, newUp, forward);
		}
		mat.SetTranslation(location);
	}

}
