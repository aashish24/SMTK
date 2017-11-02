<?xml version="1.0" encoding="utf-8" ?>
<!-- Description of the "elevate mesh" Operator -->
<SMTK_AttributeSystem Version="2">
  <Definitions>
    <!-- Operator -->
    <AttDef Type="elevate mesh"
            Label="Mesh - Apply Elevation" BaseType="operator">
      <BriefDescription>
        Modify the z-coordinates a mesh's nodes according to an external data set.
      </BriefDescription>
      <DetailedDescription>
        &lt;p&gt;Modify the z-coordinates a mesh's nodes according to an
        external data set.
        &lt;p&gt;This operator accepts as input a mesh and an external
        data set, and it computes new z-coordinates at each mesh node
        as a radial average of the scalar values in the external data set.
      </DetailedDescription>
      <ItemDefinitions>

        <String Name="input data" Label="Input Data">

          <BriefDescription>The input data (coordinates and scalar values) to use for elevation.</BriefDescription>
          <DetailedDescription>
            An input data set is required to elevate a mesh. It can be
            an auxiliary geometry, an input file, or a collection of
            coordinates and scalar values.
          </DetailedDescription>

          <ChildrenDefinitions>
            <ModelEntity Name="auxiliary geometry" Label = "Auxiliary Geometry" NumberOfRequiredValues="1">
              <MembershipMask>aux_geom</MembershipMask>
              <BriefDescription>
                An external data set whose values determine the mesh nodes' elevations.
              </BriefDescription>
            </ModelEntity>

            <File Name="ptsfile" Label="Input File" NumberOfValues="1"
                  ShouldExist="true" FileFilters="CSV file (*.csv);;Aux Geom Files (*.tif *.tiff *.dem *.vti *.vtp *.vtu *.vtm *.obj *.ply *.pts *.xyz);; Image files (*.tif *.tiff *.dem);;VTK files (*.vti *.vtp *.vtu *.vtm);;Wavefront OBJ files (*.obj);;Point Cloud Files (*.pts *.xyz);;Stanford Triangle Files (*.ply);;All files (*.*)">
              <BriefDescription>
                A file describing a data set whose values determine the mesh nodes' elevations.
              </BriefDescription>
            </File>

            <Group Name="points" Label="Interpolation Points"
                   Extensible="true" NumberOfRequiredGroups="1" >
              <BriefDescription>The points and their scalar values to interpolate.</BriefDescription>
              <DetailedDescription>
                An implicit function is described by interpolating points
                and their associated scalar value.
              </DetailedDescription>
              <ItemDefinitions>
                <Double Name="point" Label="Point" NumberOfRequiredValues="4">
                  <ComponentLabels>
                    <Label>X</Label>
                    <Label>Y</Label>
                    <Label>Z</Label>
                    <Label>Scalar</Label>
                  </ComponentLabels>
                </Double>
              </ItemDefinitions>
            </Group>
          </ChildrenDefinitions>

          <DiscreteInfo DefaultIndex="0">
	    <Structure>
              <Value Enum="Auxiliary Geometry">auxiliary geometry</Value>
	      <Items>
		<Item>auxiliary geometry</Item>
	      </Items>
	    </Structure>
	    <Structure>
              <Value Enum="Input File">ptsfile</Value>
	      <Items>
	        <Item>ptsfile</Item>
	      </Items>
	    </Structure>
	    <Structure>
              <Value Enum="Interpolation Points">points</Value>
	      <Items>
	        <Item>points</Item>
	      </Items>
	    </Structure>
          </DiscreteInfo>
        </String>

        <MeshEntity Name="mesh" Label="Mesh" NumberOfRequiredValues="1" Extensible="true" >
          <BriefDescription>The mesh to elevate.</BriefDescription>
        </MeshEntity>

        <String Name="interpolation scheme" Label="Interpolation Scheme">

          <BriefDescription>The interpolation scheme to apply to the input data</BriefDescription>
          <DetailedDescription>
            The scalar values from the input data must be interpolated
            onto the nodes of the mesh. Currently, the supported
            techniques are Radial Average (unweighted average of all
            points within a cylinder of a given radius from a mesh
            node) and Inverse Distance Weighting (weighted average of
            all the points in the data set according to the distance
            between the datapoint and the mesh node).
          </DetailedDescription>

          <ChildrenDefinitions>

        <Double Name="radius" Label="Radius for Averaging Elevation" NumberOfRequiredValues="1" Extensible="false">
          <BriefDescription>The radius used to identify a point locus for computing elevation.</BriefDescription>
          <DetailedDescription>
            The radius used to identify a point locus for computing
            elevation.

            For each node in the input mesh, a corresponding point
            locus whose positions projected onto the x-y plane are
            within a radius of the node is used to compute a new
            elevation value.
          </DetailedDescription>
        </Double>

        <Double Name="power" Label="Weighting Power" NumberOfRequiredValues="1" Extensible="no">
          <BriefDescription>The weighting power used to interpolate source points.</BriefDescription>
          <DetailedDescription>
            The weighting power used to interpolate source points.

            The inverse distance value is exponentiated by
            this unitless value when computing weighting coefficients.
          </DetailedDescription>
          <DefaultValue>1.</DefaultValue>
        </Double>

          </ChildrenDefinitions>

          <DiscreteInfo DefaultIndex="0">
	    <Structure>
              <Value Enum="Radial Average">radial average</Value>
	      <Items>
		<Item>radius</Item>
	      </Items>
	    </Structure>
	    <Structure>
              <Value Enum="Inverse Distance Weighting">inverse distance weighting</Value>
	      <Items>
	        <Item>power</Item>
	      </Items>
	    </Structure>
          </DiscreteInfo>

        </String>

        <Double Name="max elevation" Label="Maximum Elevation"
                NumberOfRequiredValues="1" Extensible="false" Optional="true">
          <BriefDescription>Upper limit to the resulting mesh's elevation range.</BriefDescription>
          <DefaultValue>0.0</DefaultValue>
        </Double>

        <Double Name="min elevation" Label="Minimum Elevation"
                NumberOfRequiredValues="1" Extensible="false" Optional="true">
          <BriefDescription>Lower limit to the resulting mesh's elevation range.</BriefDescription>
          <DefaultValue>0.0</DefaultValue>
        </Double>

        <Void Name="invert scalars" Label="Invert Scalar Values" Version="0"
              Optional="true" IsEnabledByDefault="false" AdvanceLevel="1">
          <BriefDescription>This toggle adds a prefactor of -1 to the
          values in the external data set prior to averaging.</BriefDescription>
        </Void>

      </ItemDefinitions>
    </AttDef>
    <!-- Result -->
    <AttDef Type="result(elevate mesh)" BaseType="result">
      <ItemDefinitions>
        <MeshEntity Name="mesh_modified" NumberOfRequiredValues="0" Extensible="true" AdvanceLevel="11"/>
        <ModelEntity Name="tess_changed" NumberOfRequiredValues="0"
                     Extensible="true" AdvanceLevel="11"/>
      </ItemDefinitions>
    </AttDef>
  </Definitions>
</SMTK_AttributeSystem>
