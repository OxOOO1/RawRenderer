#include "DebugDraw.h"

#include "Renderer.h"


void RDebugDrawable::UploadInstance(InstanceDescription* instanceDesc, UINT instanceIndex)
{
	RCommandList::UploadToBufferImmediate(InstancesBuffer, instanceDesc, sizeof(InstanceDescription), sizeof(InstanceDescription) * instanceIndex);
}
