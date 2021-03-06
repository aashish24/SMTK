<?xml version="1.0" encoding="utf-8" ?>
<!-- Description of the CMB Model "ModelGroup" Operation -->
<SMTK_AttributeResource Version="3">
  <Definitions>
    <!-- Operation -->
    <include href="smtk/operation/Operation.xml"/>
    <AttDef Type="entity group" Label="Model - Create Group" BaseType="operation">
      <BriefDescription>
        Create a group of cell entities. User can modify and remove the created group afterwards.
      </BriefDescription>
      <DetailedDescription>
        Create a group of cell entities. User can modify and remove the created group afterwards.

        If advance level is turned on, user can filter the entities by their cell type then use them to modify the group.
      </DetailedDescription>
      <AssociationsDef Name="model" NumberOfRequiredValues="1">
        <BriefDescription>The model which does/will hold the edited/created group.</BriefDescription>
        <Accepts><Resource Name="smtk::model::Resource" Filter="model"/></Accepts>
      </AssociationsDef>
      <ItemDefinitions>
        <String Name="Operation" Label="Operation" Version="0" AdvanceLevel="0" NumberOfRequiredValues="1">
          <BriefDescription>
            The operation determines which action to take on the group: create it, modify its membership, or remove it.
          </BriefDescription>
          <DetailedDescription>
            The operation determines which action to take on the group: create it, modify its membership, or remove it.
          </DetailedDescription>
          <ChildrenDefinitions>
            <Component Name="modify cell group" Label="modify entity group" NumberOfRequiredValues="1">
              <Accepts><Resource Name="smtk::model::Resource" Filter="group"/></Accepts>
            </Component>
            <Component Name="remove cell group" Label="remove entity group" Extensible="1" NumberOfRequiredValues="0">
              <Accepts><Resource Name="smtk::model::Resource" Filter="group"/></Accepts>
            </Component>
            <Component Name="cell to add" Label="entity to add" NumberOfRequiredValues="0" Extensible="1">
              <Accepts><Resource Name="smtk::model::Resource" Filter="volume|face|edge|vertex"/></Accepts>
            </Component>
            <Component Name="cell to remove" Label="entity to remove" NumberOfRequiredValues="0" Extensible="1">
              <Accepts><Resource Name="smtk::model::Resource" Filter="volume|face|edge|vertex"/></Accepts>
            </Component>
            <Void Name="Vertex" Label="Vertex" Version="0" NumberOfRequiredValues="1" Optional="true" AdvanceLevel = "1" Option = "true" IsEnabledByDefault = "true">
              <BriefDescription>Allow vertices to be added to the group.</BriefDescription>
              <DetailedDescription>Allow vertices to be added to the group.</DetailedDescription>
            </Void>
            <Void Name="Edge" Label="Edge" Version="0" NumberOfRequiredValues="1" Optional="true" AdvanceLevel = "1" Option = "true" IsEnabledByDefault = "true">
              <BriefDescription>Allow edges to be added to the group.</BriefDescription>
              <DetailedDescription>Allow edges to be added to the group.</DetailedDescription>
            </Void>
            <Void Name="Face" Label="Face" Version="0" NumberOfRequiredValues="1" Optional="true" AdvanceLevel = "1" Option = "true" IsEnabledByDefault = "true">
              <BriefDescription>Allow faces to be added to the group.</BriefDescription>
              <DetailedDescription>Allow faces to be added to the group.</DetailedDescription>
            </Void>
            <Void Name="Volume" Label="Volume" Version="0" NumberOfRequiredValues="1" Optional="true" AdvanceLevel = "1" Option = "true" IsEnabledByDefault = "true">
              <BriefDescription>Allow volumes to be added to the group.</BriefDescription>
              <DetailedDescription>Allow volumes to be added to the group.</DetailedDescription>
            </Void>
            <String Name="group name" Label="group name" Version="0" AdvanceLevel="0" NumberOfRequiredValues="1">
              <DefaultValue>new group</DefaultValue>
            </String>
          </ChildrenDefinitions>

          <DiscreteInfo DefaultIndex="0">
            <Structure>
              <Value Enum="Create Group">Create</Value>
              <Items>
                <Item>group name</Item>
                <Item>cell to add</Item>
                <Item>Vertex</Item>
                <Item>Edge</Item>
                <Item>Face</Item>
                <Item>Volume</Item>
              </Items>
            </Structure>
            <Structure>
              <Value Enum="Remove Group">Remove</Value>
              <Items>
                <Item>remove cell group</Item>
              </Items>
            </Structure>
            <Structure>
              <Value Enum="Modify Group">Modify</Value>
              <Items>
                <Item>modify cell group</Item>
                <Item>cell to add</Item>
                <Item>cell to remove</Item>
                <Item>Vertex</Item>
                <Item>Edge</Item>
                <Item>Face</Item>
                <Item>Volume</Item>
              </Items>
            </Structure>
          </DiscreteInfo>
        </String>
      </ItemDefinitions>
    </AttDef>

    <!-- Result -->
    <include href="smtk/operation/Result.xml"/>
    <AttDef Type="result(entity group)" BaseType="result">
      <ItemDefinitions>
      </ItemDefinitions>
    </AttDef>
  </Definitions>
</SMTK_AttributeResource>
