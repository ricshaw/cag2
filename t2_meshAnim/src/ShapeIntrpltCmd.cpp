#include "ShapeIntrpltCmd.h"
#include "ShapeIntrpltNode.h"
#include <maya/MSelectionList.h>
#include <maya/MGlobal.h>
#include <maya/MDagPath.h>
#include <maya/MFnTransform.h>
#include <maya/MItSelectionList.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MTime.h>
#include <maya/MAnimControl.h>
#include <assert.h>

MStatus ShapeIntrpltCmd::doIt(const MArgList &args)
{
	// This command takes the selected mesh node, and conects its output to
	// a new shape interpolation node, then creates a new mesh node whose 
	// input is the output of the interpolation node
	MStatus stat;
	MSelectionList selection;

	// Get the initial shading group
	MObject shadingGroupObj;

	// N.B. Ensure the selection is list empty beforehand since
	// getSelectionListByName() will append the matching objects
	selection.clear();

	MGlobal::getSelectionListByName("initialShadingGroup", selection);
	selection.getDependNode(0, shadingGroupObj);
	MFnSet shadingGroupFn(shadingGroupObj);

	// Get a list of currently selected objects
	selection.clear();
	MGlobal::getActiveSelectionList(selection);
	MItSelectionList iter(selection);

	unsigned int count;

	MDagPath sourceShapePath;
	MDagPath targetShapePath;

	// Iterate over all the mesh nodes
	iter.setFilter(MFn::kMesh);
	for (iter.reset(), count = 0; !iter.isDone(); iter.next(), count++)
	{


		MDagPath geomShapePath;
		iter.getDagPath(geomShapePath);

		MDagPath geomTransformPath(geomShapePath);
		// Take the shape node out of the path
		geomTransformPath.pop();

		MFnDagNode geomShapeFn(geomShapePath);

		MGlobal::displayInfo(geomShapeFn.name());

		if (geomShapeFn.name().indexW("source") != -1){

			sourceShapePath = geomShapePath;
		}
		else if (geomShapeFn.name().indexW("target") != -1){
			targetShapePath = geomShapePath;
		}
		else {
			continue;
		}
	}

	if (count < 2)
	{
		MGlobal::displayError("\nSelect source and target shape.");
		return MS::kFailure;
	}

	// TODO Move to constructor
	newNodeName = "interpolatorNode";

	duplicateMesh(shadingGroupFn, sourceShapePath, targetShapePath);

	//MString n( meltFn.name() );
	//MGlobal::displayInfo( "\nMelt node: " + name + " " + shapeFn.name() );

	MTime startTime = MAnimControl::minTime();
	MTime endTime = MAnimControl::maxTime();

	MString cmd;
	cmd = MString("setKeyframe -at interpolateValue -t ") + startTime.value() + " -v " + 0.0 + " " + newNodeName;
	dgMod.commandToExecute(cmd);

	cmd = MString("setKeyframe -at interpolateValue -t ") + endTime.value() + " -v " + 1.0 + " " + newNodeName;
	dgMod.commandToExecute(cmd);

	return redoIt();
}

MStatus ShapeIntrpltCmd::undoIt()
{
	MStatus status;

	if (doMeshUndo)
	{
		MGlobal::deleteNode(newMesh);
		doMeshUndo = false;
	}

	return dgMod.undoIt();
}

MStatus ShapeIntrpltCmd::redoIt()
{
	return dgMod.doIt();
}

void ShapeIntrpltCmd::duplicateMesh(MFnSet &shadingGroupFn, MDagPath &geomShapePath,
	MDagPath &targetShapePath){

	MFnDagNode geomShapeFn(geomShapePath);

	// Duplicate the mesh
	// duplicate(bool instance = false, bool instaceLeaf = false)
	// Set to true to create instances, instanceLeaf controls whether the instance is
	// only at the leaf, returns a reference to the new Node
	MObject newGeomTransform = geomShapeFn.duplicate(false, false);

	// Save the dag path of the newly created mesh
	newMesh = newGeomTransform;
	doMeshUndo = true;

	// Add the mesh with the new shape node to the 
	MFnDagNode newGeomShapeFn(newGeomTransform);

	newGeomShapeFn.setObject(newGeomShapeFn.child(0));

	// Assign the new surface to the shading group or it won't be shown
	shadingGroupFn.addMember(newGeomShapeFn.object());

	// Create ShapeIntrpltNode node
	MObject intrpltNode = dgMod.createNode(ShapeIntrpltNode::id);
	assert(!intrpltNode.isNull());

	// Give the node a custom name
	dgMod.renameNode(intrpltNode, newNodeName);

	MFnDependencyNode intrpltNodeFn(intrpltNode);

	MPlug sourceSurfacePlug = intrpltNodeFn.findPlug("sourceSurface");
	MPlug targetSurfacePlug = intrpltNodeFn.findPlug("targetSurface");
	MPlug outputSurfacePlug = intrpltNodeFn.findPlug("outputSurface");

	MFnDagNode targetShapeFn(targetShapePath);
	MPlug outGeomPlug = geomShapeFn.findPlug("worldMesh");
	MPlug targetPlug = targetShapeFn.findPlug("worldMesh");

	unsigned int instanceNum = geomShapePath.instanceNumber();

	// Set the plug to the correct element in the array
	outGeomPlug.selectAncestorLogicalIndex(instanceNum);

	instanceNum = targetShapePath.instanceNumber();
	targetPlug.selectAncestorLogicalIndex(instanceNum);

	MPlug inGeomPlug = newGeomShapeFn.findPlug("inMesh");

	dgMod.connect(outGeomPlug, sourceSurfacePlug);
	dgMod.connect(targetPlug, targetSurfacePlug);
	dgMod.connect(outputSurfacePlug, inGeomPlug);
}