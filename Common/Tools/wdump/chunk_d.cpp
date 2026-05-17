/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**  Copyright 2026 Stephan Vedder
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : WDump                                                        *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Tools/W3DShellExt/External/chunk_d.cpp       $*
 *                                                                                             *
 *                      $Author:: Moumine_ballo                                               $*
 *                                                                                             *
 *                     $Modtime:: 1/02/02 1:22p                                               $*
 *                                                                                             *
 *                    $Revision:: 66                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include "chunk_d.h"
#include "rawfile.h"
#include "w3d_obsolete.h"
#include <stdio.h>

ChunkModel::ChunkModel(QObject *parent) : QAbstractItemModel(parent) {}

ChunkModel::~ChunkModel() {}

QModelIndex ChunkModel::index(int row, int column, const QModelIndex &parent) const
{
	if (row < 0 || column < 0 || row >= m_items.size() || column >= 3)
		return QModelIndex();

	auto it = m_items.begin();
	std::advance(it, row);
	return createIndex(row, column, &(*it));
}

QModelIndex ChunkModel::parent(const QModelIndex &index) const
{
	return QModelIndex(); // Flat list, no parent-child relationships
}

int ChunkModel::rowCount(const QModelIndex &parent) const
{
	return m_items.size();
}

int ChunkModel::columnCount(const QModelIndex &parent) const
{
	return 3; // Name, Type, Value
}

QVariant ChunkModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || role != Qt::DisplayRole)
		return QVariant();

	auto it = m_items.begin();
	std::advance(it, index.row());
	const ChunkModelItem &item = *it;

	switch (index.column())
	{
	case 0: return item.Name;
	case 1: return item.Type;
	case 2: return item.Value;
	default: return QVariant();
	}
}

QVariant ChunkModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		switch (section)
		{
		case 0: return QString("Name");
		case 1: return QString("Type");
		case 2: return QString("Value");
		default: return QVariant();
		}
	}
	return QVariant();
}

void ChunkModel::addItem(const QString &name, const QString &type, const QVariant &value)
{
	m_items.push_back({name, type, value});
}

static int Get_Bit(void const * array, int bit);
static void Set_Bit(void * array, int bit, int value);


ChunkTableClass ChunkItem::ChunkTable;

ChunkTableClass::~ChunkTableClass() {
    for (auto &[key, chunktype] : Types) {
        delete chunktype;
    }
}
void ChunkTableClass::NewType(int ID, const char *name, void (*callback)(ChunkItem *item, ChunkModel *model), bool wrapper) {
	ChunkType *chunktype = new ChunkType(name, callback, wrapper);
	Types[ID] = chunktype;
}
void ChunkTableClass::AddItem(ChunkModel *model, const char *Name, const char *Value, const char *Type) {

	if (model != nullptr) {
        model->addItem(Name, Type, Value);

	} else {
		fprintf(stderr, "Model is null, cannot add item %s\n", Name);
	}
}

void ChunkTableClass::AddItemVersion(ChunkModel *model, uint32 version)
{
	char buf[64];
	sprintf(buf,"%d.%d",W3D_GET_MAJOR_VERSION(version),W3D_GET_MINOR_VERSION(version));
	AddItem(model,"Version",buf);
}

void ChunkTableClass::AddItem(ChunkModel *model, const char *Name, uint32 Value) {
	char buf[256];
	sprintf(buf, "%d", Value);
	AddItem(model, Name, buf, "int32");
}
void ChunkTableClass::AddItem(ChunkModel *model, const char *Name, uint16 Value) {
	char buf[256];
	sprintf(buf, "%d", Value);
	AddItem(model, Name, buf, "int16");
}
void ChunkTableClass::AddItem(ChunkModel *model, const char *Name, uint8 Value) {
	char buf[256];
	sprintf(buf, "%d", Value);
	AddItem(model, Name, buf, "int8");
}
void ChunkTableClass::AddItem(ChunkModel *model, const char *Name, uint8 *Value, int Count) {
	QString buffer;
    int counter = 0;
	
	while(counter < Count) {
		buffer += QString("%1 ").arg(Value[counter++]);
	}
	char type[256];
	sprintf(type, "int8[%d]", Count);
	AddItem(model, Name, buffer, type);
}
void ChunkTableClass::AddItem(ChunkModel *model, const char *Name, float32 Value) {
	char buf[256];
	sprintf(buf, "%f", Value);
	AddItem(model, Name, buf, "float");
}
void ChunkTableClass::AddItem(ChunkModel *model, const char *Name, uint32 *Value, int Count) {
	QString buffer;
    int counter = 0;
	
	while(counter < Count) {
		buffer += QString("%1 ").arg(Value[counter++]);
	}
	char type[256];
	sprintf(type, "int32[%d]", Count);
	AddItem(model, Name, buffer, type);
}
void ChunkTableClass::AddItem(ChunkModel *model, const char *Name, float32 *Value, int Count) {
	QString buffer;
    int counter = 0;
	
	while(counter < Count) {
		buffer += QString("%1 ").arg(Value[counter++]);
	}
	char type[256];
	sprintf(type, "float[%d]", Count);
	AddItem(model, Name, buffer, type);
}

void ChunkTableClass::AddItem(ChunkModel *model, const char *Name, IOVector3Struct *Value) {
	char buf[256];
	sprintf(buf, "%f %f %f", Value->X, Value->Y, Value->Z);
	AddItem(model, Name, buf, "vector");
}

void ChunkTableClass::AddItem(ChunkModel *model, const char *Name, IOVector4Struct *Value) {
	char buf[256];
	sprintf(buf, "%f %f %f %f", Value->X, Value->Y, Value->Z, Value->W);
	AddItem(model, Name, buf, "vector4");
}

void ChunkTableClass::AddItem(ChunkModel *model, const char *Name, W3dQuaternionStruct *Value) {
	char buf[256];
	sprintf(buf, "%f %f %f %f", Value->Q[0], Value->Q[1], Value->Q[2], Value->Q[3]);
	AddItem(model, Name, buf, "quaternion");
}

void ChunkTableClass::AddItem(ChunkModel *model, const char *Name, W3dRGBStruct *Value) {
	QString buffer;
	buffer = QString("(%1 %2 %3) ").arg(Value->R).arg(Value->G).arg(Value->B);
	AddItem(model, Name, buffer, "RGB");
}

void ChunkTableClass::AddItem(ChunkModel *model, const char *Name, W3dRGBStruct *Value, int Count) {
	QString buffer;
    int counter = 0;
	
	while(counter < Count) {
		buffer += QString("(%1 %2 %3) ").arg(Value[counter].R).arg(Value[counter].G).arg(Value[counter].B);
		counter++;
	}
	char type[256];
	sprintf(type, "RGB[%d]", Count);
	AddItem(model, Name, buffer, type);
}

void ChunkTableClass::AddItem(ChunkModel *model, const char *Name, W3dRGBAStruct *Value) {
	QString buffer;
    buffer = QString("(%1 %2 %3 %4) ").arg(Value->R).arg(Value->G).arg(Value->B).arg(Value->A);
	AddItem(model, Name, buffer, "RGBA");
}

void ChunkTableClass::AddItem(ChunkModel *model, const char *Name, W3dTexCoordStruct *Value, int Count) {
	QString temp;
    int counter = 0;
	
	while(counter < Count) {
		char type[256];
		sprintf(type, "%s.TexCoord[%d]", Name, counter);
		AddItem(model, type, &Value[counter]);
		counter++;
	}
}

void ChunkTableClass::AddItem(ChunkModel *model, const char *Name, W3dTexCoordStruct *Value) 
{
	char buf[256];
	sprintf(buf, "%f %f", Value->U, Value->V);
	AddItem(model, Name, buf, "UV");
}

void ChunkTableClass::AddItem(ChunkModel *model, const char *name, W3dShaderStruct * shader) 
{
	static const char * _depth_compare[] = { "Pass Never","Pass Less","Pass Equal","Pass Less or Equal", "Pass Greater","Pass Not Equal","Pass Greater or Equal","Pass Always" };
	static const char * _depth_mask[] = { "Write Disable", "Write Enable" };
	static const char * _color_mask[] = { "Write Disable", "Write Enable" };
	static const char * _destblend[] = { "Zero","One","Src Color","One Minus Src Color","Src Alpha","One Minus Src Alpha","Src Color Prefog" };
	static const char * _fogfunc[] = { "Disable","Enable","Scale Fragment","Replace Fragment" };
	static const char * _prigradient[] = { "Disable","Modulate","Add","Bump-Environment" };
	static const char * _secgradient[] = { "Disable","Enable" };
	static const char * _srcblend[] = { "Zero","One","Src Alpha","One Minus Src Alpha" };
	static const char * _texturing[] = { "Disable","Enable" };
	static const char * _detailcolor[] = { "Disable","Detail","Scale","InvScale","Add","Sub","SubR","Blend","DetailBlend" };
	static const char * _detailalpha[] = { "Disable","Detail","Scale","InvScale" };
	static const char * _dithermask[] = { "Disable", "Enable" };
	static const char * _shademodel[] = { "Smooth", "Flat" };
	static const char * _alphatest[] = { "Alpha Test Disable", "Alpha Test Enable" };

	
	char label[256];

	sprintf(label,"%s.DepthCompare",name);
	AddItem(model, label, _depth_compare[W3d_Shader_Get_Depth_Compare(shader)]);
	sprintf(label,"%s.DepthMask",name);
	AddItem(model, label, _depth_mask[W3d_Shader_Get_Depth_Mask(shader)]);
	sprintf(label,"%s.DestBlend",name);
	AddItem(model, label, _destblend[W3d_Shader_Get_Dest_Blend_Func(shader)]);
	sprintf(label,"%s.PriGradient",name);
	AddItem(model, label, _prigradient[W3d_Shader_Get_Pri_Gradient(shader)]);
	sprintf(label,"%s.SecGradient",name);
	AddItem(model, label, _secgradient[W3d_Shader_Get_Sec_Gradient(shader)]);
	sprintf(label,"%s.SrcBlend",name);
	AddItem(model, label, _srcblend[W3d_Shader_Get_Src_Blend_Func(shader)]);
	sprintf(label,"%s.Texturing",name);
	AddItem(model, label, _texturing[W3d_Shader_Get_Texturing(shader)]);
	sprintf(label,"%s.DetailColor",name);
	AddItem(model, label, _detailcolor[W3d_Shader_Get_Detail_Color_Func(shader)]);
	sprintf(label,"%s.DetailAlpha",name);
	AddItem(model, label, _detailalpha[W3d_Shader_Get_Detail_Alpha_Func(shader)]);
	sprintf(label,"%s.AlphaTest",name);
	AddItem(model, label, _alphatest[W3d_Shader_Get_Alpha_Test(shader)]);	
}

void ChunkTableClass::AddItem(ChunkModel *model, const char *name, W3dPS2ShaderStruct * shader) 
{
	static const char * _depth_compare[] = { "Pass Never","Pass Less","Pass Always","Pass Less or Equal"};
	static const char * _depth_mask[] = { "Write Disable", "Write Enable" };
	static const char * _color_mask[] = { "Write Disable", "Write Enable" };
	static const char * _ablend[] = { "Src Color","Dest Color","Zero"};
	static const char * _cblend[] = { "Src Alpha","Dest Alpha","One"};
	static const char * _fogfunc[] = { "Disable","Enable","Scale Fragment","Replace Fragment" };
	static const char * _prigradient[] = { "Disable","Modulate","Highlight","Highlight2" };
	static const char * _secgradient[] = { "Disable","Enable" };
	static const char * _texturing[] = { "Disable","Enable" };
	static const char * _detailcolor[] = { "Disable","Detail","Scale","InvScale","Add","Sub","SubR","Blend","DetailBlend" };
	static const char * _detailalpha[] = { "Disable","Detail","Scale","InvScale" };
	static const char * _dithermask[] = { "Disable", "Enable" };
	static const char * _shademodel[] = { "Smooth", "Flat" };

	
	char label[256];

	sprintf(label,"%s.DepthCompare",name);
	AddItem(model, label, _depth_compare[W3d_Shader_Get_Depth_Compare(shader)]);
	sprintf(label,"%s.DepthMask",name);
	AddItem(model, label, _depth_mask[W3d_Shader_Get_Depth_Mask(shader)]);
	sprintf(label,"%s.PriGradient",name);
	AddItem(model, label, _prigradient[W3d_Shader_Get_Pri_Gradient(shader)]);
	sprintf(label,"%s.Texturing",name);
	AddItem(model, label, _texturing[W3d_Shader_Get_Texturing(shader)]);

	sprintf(label,"%s.AParam",name);
	AddItem(model, label, _ablend[W3d_Shader_Get_PS2_Param_A(shader)]);

	sprintf(label,"%s.BParam",name);
	AddItem(model, label, _ablend[W3d_Shader_Get_PS2_Param_B(shader)]);

	sprintf(label,"%s.CParam",name);
	AddItem(model, label, _cblend[W3d_Shader_Get_PS2_Param_C(shader)]);

	sprintf(label,"%s.DParam",name);
	AddItem(model, label, _ablend[W3d_Shader_Get_PS2_Param_D(shader)]);

	shader++;
}


void ChunkTableClass::AddItem(ChunkModel *Model, const char *Name, Vector3i *Value) {
	char buf[256];
	sprintf(buf, "%d %d %d", Value->I, Value->J, Value->K);
	AddItem(Model, Name, buf, "IJK");
}

void ChunkTableClass::List_Subitems(ChunkItem *Item, ChunkModel *Model) {
	for(ChunkItem *subitem : Item->Chunks) {
		if(subitem->Type) {
			AddItem(Model, subitem->Type->Name,  "", "chunk");
		}
	}
}
void ChunkTableClass::List_W3D_CHUNK_MESH(ChunkItem *Item, ChunkModel *Model) {
	List_Subitems(Item, Model);
}
void ChunkTableClass::List_W3D_CHUNK_MESH_HEADER(ChunkItem *Item, ChunkModel *Model) {

	struct W3dMeshHeaderStruct *data;
	data = (W3dMeshHeaderStruct *) Item->Data;
	
	char buf[64];
	sprintf(buf,"%d.%d",W3D_GET_MAJOR_VERSION(data->Version),W3D_GET_MINOR_VERSION(data->Version));
	AddItem(Model, "Version", buf);
	AddItem(Model, "MeshName", data->MeshName);
	AddItem(Model,"Attributes",data->Attributes);
	if (data->Attributes & W3D_MESH_FLAG_COLLISION_BOX)	AddItem(Model, "Attributes", "W3D_MESH_FLAG_COLLISION_BOX");
	if (data->Attributes & W3D_MESH_FLAG_SKIN)	AddItem(Model, "Attributes", "W3D_MESH_FLAG_SKIN");
	if (data->Attributes & W3D_MESH_FLAG_SHADOW)	AddItem(Model, "Attributes", "W3D_MESH_FLAG_SHADOW");
	if (data->Attributes & W3D_MESH_FLAG_ALIGNED)	AddItem(Model, "Attributes", "W3D_MESH_FLAG_ALIGNED");
	if (data->Attributes & W3D_MESH_FLAG_COLLISION_TYPE_PHYSICAL)	AddItem(Model, "Attributes", "W3D_MESH_FLAG_COLLISION_TYPE_PHYSICAL");
	if (data->Attributes & W3D_MESH_FLAG_COLLISION_TYPE_PROJECTILE)	AddItem(Model, "Attributes", "W3D_MESH_FLAG_COLLISION_TYPE_PROJECTILE");
	AddItem(Model, "NumTris", data->NumTris);
	AddItem(Model, "NumQuads", data->NumQuads);
	AddItem(Model, "NumSrTris", data->NumSrTris);
	AddItem(Model, "NumPovQuads", data->NumPovQuads);
	AddItem(Model, "NumVertices", data->NumVertices);
	AddItem(Model, "NumNormals", data->NumNormals);
	AddItem(Model, "NumSrNormals", data->NumSrNormals);
	AddItem(Model, "NumTexCoords", data->NumTexCoords);
	AddItem(Model, "NumMaterials", data->NumMaterials);
	AddItem(Model, "NumVertColors", data->NumVertColors);
	AddItem(Model, "NumVertInfluences", data->NumVertInfluences);
	AddItem(Model, "NumDamageStages", data->NumDamageStages);
	AddItem(Model, "FutureCounts", data->FutureCounts, 5);
	AddItem(Model, "LODMin", data->LODMin);
	AddItem(Model, "LODMax", data->LODMax);
	AddItem(Model, "Min", &data->Min);
	AddItem(Model, "Max", &data->Max);
	AddItem(Model, "SphCenter", &data->SphCenter);
	AddItem(Model, "SphRadius", data->SphRadius);
	AddItem(Model, "Translation", &data->Translation);
	AddItem(Model, "Rotation", data->Rotation, 9);
	AddItem(Model, "MassCenter", &data->MassCenter);
	AddItem(Model, "Inertia", data->Inertia, 9);
	AddItem(Model, "Volume", data->Volume);
	AddItem(Model, "HierarchyTreeName", data->HierarchyTreeName);
	AddItem(Model, "HierarchyModelName", data->HierarchyModelName);
	AddItem(Model, "FutureUse", data->FutureUse, 24);
}
void ChunkTableClass::List_W3D_CHUNK_VERTICES(ChunkItem *Item, ChunkModel *Model) {
	W3dVectorStruct *data;
	data = (W3dVectorStruct *) Item->Data;
	
	int counter = 0;
	void *max = (char *) Item->Data + Item->Length;
	
	char buf[256];
	while(data < max) {
		sprintf(buf, "Vertex[%d]", counter++);
		AddItem(Model, buf, data);
		data++;
	}
}
void ChunkTableClass::List_W3D_CHUNK_VERTEX_NORMALS(ChunkItem *Item, ChunkModel *Model) {
	W3dVectorStruct *data;
	data = (W3dVectorStruct *) Item->Data;
	
	void *max = (char *) Item->Data + Item->Length;
	int counter = 0;
	char buf[256];
	while(data < max) {
		sprintf(buf, "Normal[%d]", counter++);
		AddItem(Model, buf, data);
		data++;
	}
}
void ChunkTableClass::List_W3D_CHUNK_SURRENDER_NORMALS(ChunkItem *Item, ChunkModel *Model) {
	W3dVectorStruct *data;
	data = (W3dVectorStruct *) Item->Data;
	
	void *max = (char *) Item->Data + Item->Length;
	int counter = 0;
	char buf[256];
	while(data < max) {
		sprintf(buf, "SRNormal[%d]", counter++);
		AddItem(Model, buf, data);
		data++;
	}
}
void ChunkTableClass::List_W3D_CHUNK_TEXCOORDS(ChunkItem *Item, ChunkModel *Model) {
	
	W3dTexCoordStruct *data;
	data = (W3dTexCoordStruct *) Item->Data;
	
	void *max = (char *) Item->Data + Item->Length;
	int counter = 0;
	char buf[256];
	while(data < max) {
		sprintf(buf, "TexCoord[%d]", counter++);
		AddItem(Model, buf, data);
		data++;
	}
}
void ChunkTableClass::List_O_W3D_CHUNK_MATERIALS(ChunkItem *Item, ChunkModel *Model) {

	struct W3dMaterialStruct *data;
	data = (W3dMaterialStruct *) Item->Data;
	
	void *max = (char *) Item->Data + Item->Length;
	int counter = 0;
	char buf[256];
	while(data < max) {

		sprintf(buf, "Material[%d].MaterialName", counter);
		AddItem(Model, buf, data->MaterialName);

		sprintf(buf, "Material[%d].PrimaryName", counter);
		AddItem(Model, buf, data->PrimaryName);

		sprintf(buf, "Material[%d].SecondaryName", counter);
		AddItem(Model, buf, data->SecondaryName);

		sprintf(buf, "Material[%d].RenderFlags", counter);
		AddItem(Model, buf, data->RenderFlags);

		sprintf(buf, "Material[%d].Red", counter);
		AddItem(Model, buf, data->Red);

		sprintf(buf, "Material[%d].Green", counter);
		AddItem(Model, buf, data->Green);

		sprintf(buf, "Material[%d].Blue", counter);
		AddItem(Model, buf, data->Blue);

		counter++;
		data++;
	}

}


void ChunkTableClass::List_O_W3D_CHUNK_TRIANGLES(ChunkItem *Item, ChunkModel *Model) {

	
	AddItem(Model, "Obsolete structure", "");
}
void ChunkTableClass::List_O_W3D_CHUNK_QUADRANGLES(ChunkItem *Item, ChunkModel *Model) {
		
		AddItem(Model, "Outdated structure", "");
}
void ChunkTableClass::List_O_W3D_CHUNK_SURRENDER_TRIANGLES(ChunkItem *Item, ChunkModel *Model) {
	struct W3dSurrenderTriStruct *data;
	data = (W3dSurrenderTriStruct *) Item->Data;
	
	void *max = (char *) Item->Data + Item->Length;
	int counter = 0;
	char buf[256];
	while(data < max) {

		sprintf(buf, "Triangle[%d].Attributes", counter);
		AddItem(Model, buf, data->Attributes);

		sprintf(buf, "Triangle[%d].Gouraud", counter);
		AddItem(Model, buf, data->Gouraud, 3);

		sprintf(buf, "Triangle[%d].VertexIndices", counter);
		AddItem(Model, buf, data->Vindex, 3);

		sprintf(buf, "Triangle[%d].MaterialIdx", counter);
		AddItem(Model, buf, data->MaterialIdx);

		sprintf(buf, "Triangle[%d].Normal", counter);
		AddItem(Model, buf, &data->Normal);

		sprintf(buf, "Triangle[%d].TexCoord", counter);
		AddItem(Model, buf, data->TexCoord, 3);

		counter++;
		data++;
	}

}
void ChunkTableClass::List_O_W3D_CHUNK_POV_TRIANGLES(ChunkItem *Item, ChunkModel *Model) {
	AddItem(Model, "Contact Greg if you need to look at this!", "unsupported");
}
void ChunkTableClass::List_O_W3D_CHUNK_POV_QUADRANGLES(ChunkItem *Item, ChunkModel *Model) {
	AddItem(Model, "Contact Greg if you need to look at this!", "unsupported");
}
void ChunkTableClass::List_W3D_CHUNK_MESH_USER_TEXT(ChunkItem *Item, ChunkModel *Model) {
	AddItem(Model, "UserText", (char *) Item->Data);
}
void ChunkTableClass::List_W3D_CHUNK_VERTEX_COLORS(ChunkItem *Item, ChunkModel *Model) {

	struct W3dRGBStruct *data;
	data = (W3dRGBStruct *) Item->Data;
	
	void *max = (char *) Item->Data + Item->Length;
	int counter = 0;
	char buf[256];

	int sz = sizeof(W3dRGBStruct);

	while(data < max) {
	
		sprintf(buf, "Vertex[%d].RGB", counter);
		AddItem(Model, buf, data);

		counter++;
		data++;
	}
}

void ChunkTableClass::List_W3D_CHUNK_VERTEX_INFLUENCES(ChunkItem *Item, ChunkModel *Model) {

	struct W3dVertInfStruct *data;
	data = (W3dVertInfStruct *) Item->Data;
	
	void *max = (char *) Item->Data + Item->Length;
	int counter = 0;
	char buf[256];
	while(data < max) {
	
		sprintf(buf, "VertexInfluence[%d].BoneIdx", counter);
		AddItem(Model, buf, data->BoneIdx);
		sprintf(buf, "VertexInfluence[%d].Pad", counter);
		AddItem(Model, buf, data->Pad, 6);

		counter++;
		data++;
	}
}

void ChunkTableClass::List_W3D_CHUNK_DAMAGE(ChunkItem *Item, ChunkModel *Model) {
	List_Subitems(Item, Model);
}
void ChunkTableClass::List_W3D_CHUNK_DAMAGE_HEADER(ChunkItem *Item, ChunkModel *Model) {

	struct W3dMeshDamageStruct *data;
	data = (W3dMeshDamageStruct *) Item->Data;
	
	void *max = (char *) Item->Data + Item->Length;
	int counter = 0;
	char buf[256];
	while(data < max) {
	
		sprintf(buf, "DamageStruct[%d].NumDamageMaterials", counter);
		AddItem(Model, buf, data->NumDamageMaterials);
		sprintf(buf, "DamageStruct[%d].NumDamageVerts", counter);
		AddItem(Model, buf, data->NumDamageVerts);
		sprintf(buf, "DamageStruct[%d].NumDamageColors", counter);
		AddItem(Model, buf, data->NumDamageColors);
		sprintf(buf, "DamageStruct[%d].DamageIndex", counter);
		AddItem(Model, buf, data->DamageIndex);

		counter++;
		data++;
	}
}

void ChunkTableClass::List_W3D_CHUNK_DAMAGE_VERTICES(ChunkItem *Item, ChunkModel *Model) {

	struct W3dMeshDamageVertexStruct *data;
	data = (W3dMeshDamageVertexStruct *) Item->Data;
	
	int counter = 0;
	void *max = (char *) Item->Data + Item->Length;
	
	char buf[256];
	while(data < max) {
	
		sprintf(buf, "DamageVertexStruct[%d].VertexIndex", counter);
		AddItem(Model, buf, data->VertexIndex);

		sprintf(buf, "DamageVertexStruct[%d].NewVertex", counter);
		AddItem(Model, buf, data->VertexIndex);

		counter++;
		data++;
	}
}

void ChunkTableClass::List_W3D_CHUNK_DAMAGE_COLORS(ChunkItem *Item, ChunkModel *Model) {

	struct W3dMeshDamageColorStruct *data;
	data = (W3dMeshDamageColorStruct *) Item->Data;
	
	int counter = 0;
	void *max = (char *) Item->Data + Item->Length;
	
	char buf[256];
	while(data < max) {
	
		sprintf(buf, "DamageColorStruct[%d].VertexIndex", counter);
		AddItem(Model, buf, data->VertexIndex);

		sprintf(buf, "DamageColorStruct[%d].NewColor", counter);
		AddItem(Model, buf, &data->NewColor);

		counter++;
		data++;
	}
}

void ChunkTableClass::List_O_W3D_CHUNK_MATERIALS2(ChunkItem *Item, ChunkModel *Model) {
	struct W3dMaterial2Struct *data;
	data = (W3dMaterial2Struct *) Item->Data;
	int counter = 0;
	void *max = (char *) Item->Data + Item->Length;
	
	char buf[256];
	while(data < max) {

		sprintf(buf, "Material[%d].MaterialName", counter);
		AddItem(Model, buf, data->MaterialName);

		sprintf(buf, "Material[%d].PrimaryName", counter);
		AddItem(Model, buf, data->PrimaryName);

		sprintf(buf, "Material[%d].SecondaryName", counter);
		AddItem(Model, buf, data->SecondaryName);

		sprintf(buf, "Material[%d].RenderFlags", counter);
		AddItem(Model, buf, data->RenderFlags);

		sprintf(buf, "Material[%d].Red", counter);
		AddItem(Model, buf, data->Red);

		sprintf(buf, "Material[%d].Green", counter);
		AddItem(Model, buf, data->Green);

		sprintf(buf, "Material[%d].Blue", counter);
		AddItem(Model, buf, data->Blue);
	
		sprintf(buf, "Material[%d].Alpha", counter);
		AddItem(Model, buf, data->Alpha);

		sprintf(buf, "Material[%d].PrimaryNumFrames", counter);
		AddItem(Model, buf, data->PrimaryNumFrames);

		sprintf(buf, "Material[%d].SecondaryNumFrames", counter);
		AddItem(Model, buf, data->SecondaryNumFrames);

		counter++;
		data++;
	}

}

void ChunkTableClass::List_W3D_CHUNK_MATERIALS3(ChunkItem *Item, ChunkModel* Model)
{
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_MATERIAL3(ChunkItem *Item, ChunkModel* Model)
{
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_MATERIAL3_NAME(ChunkItem *Item, ChunkModel* Model)
{
	
	char * data = (char *)Item->Data;
	AddItem(Model,"Material Name:", data);
}

void ChunkTableClass::List_W3D_CHUNK_MATERIAL3_INFO(ChunkItem *Item, ChunkModel* Model)
{
	
	struct W3dMaterial3Struct *data;
	data = (W3dMaterial3Struct *) Item->Data;
	AddItem(Model,"Attributes",data->Attributes);
	if (data->Attributes & W3DMATERIAL_USE_ALPHA) AddItem(Model,"Attributes","W3DMATERIAL_USE_ALPHA");
	if (data->Attributes & W3DMATERIAL_USE_SORTING) AddItem(Model,"Attributes","W3DMATERIAL_USE_SORTING");
	if (data->Attributes & W3DMATERIAL_HINT_DIT_OVER_DCT) AddItem(Model,"Attributes","W3DMATERIAL_HINT_DIT_OVER_DCT");
	if (data->Attributes & W3DMATERIAL_HINT_SIT_OVER_SCT) AddItem(Model,"Attributes","W3DMATERIAL_HINT_SIT_OVER_SCT");
	if (data->Attributes & W3DMATERIAL_HINT_DIT_OVER_DIG) AddItem(Model,"Attributes","W3DMATERIAL_HINT_DIT_OVER_DIG");
	if (data->Attributes & W3DMATERIAL_HINT_SIT_OVER_SIG) AddItem(Model,"Attributes","W3DMATERIAL_HINT_SIT_OVER_SIG");
	if (data->Attributes & W3DMATERIAL_HINT_FAST_SPECULAR_AFTER_ALPHA) AddItem(Model,"Attributes","W3DMATERIAL_HINT_FAST_SPECULAR_AFTER_ALPHA");
	if (data->Attributes & W3DMATERIAL_PSX_TRANS_100) AddItem(Model,"Attributes","W3DMATERIAL_PSX_TRANS_100");
	if (data->Attributes & W3DMATERIAL_PSX_TRANS_50) AddItem(Model,"Attributes","W3DMATERIAL_PSX_TRANS_50");
	if (data->Attributes & W3DMATERIAL_PSX_TRANS_25) AddItem(Model,"Attributes","W3DMATERIAL_PSX_TRANS_25");
	if (data->Attributes & W3DMATERIAL_PSX_TRANS_MINUS_100) AddItem(Model,"Attributes","W3DMATERIAL_PSX_TRANS_MINUS_100");
	if (data->Attributes & W3DMATERIAL_PSX_NO_RT_LIGHTING) AddItem(Model,"Attributes","W3DMATERIAL_PSX_NO_RT_LIGHTING");
	AddItem(Model,"Diffuse Color", &data->DiffuseColor);
	AddItem(Model,"Specular Color", &data->SpecularColor);
	AddItem(Model,"Emissive Coefficients", &data->EmissiveCoefficients);
	AddItem(Model,"Ambient Coefficients", &data->AmbientCoefficients);
	AddItem(Model,"Diffuse Coefficients", &data->DiffuseCoefficients);
	AddItem(Model,"Specular Coefficients", &data->SpecularCoefficients);
	AddItem(Model,"Shininess", data->Shininess);
	AddItem(Model,"Opacity", data->Opacity);
	AddItem(Model,"Translucency", data->Translucency);
	AddItem(Model,"Fog Coefficient", data->FogCoeff);
}

void ChunkTableClass::List_W3D_CHUNK_MAP3_FILENAME(ChunkItem * Item, ChunkModel* Model)
{
	char * data = (char *)Item->Data;
	AddItem(Model, "Texture Filename:", data);
}

void ChunkTableClass::List_W3D_CHUNK_MAP3_INFO(ChunkItem * Item, ChunkModel* Model)
{
	struct W3dMap3Struct * data;
	data = (W3dMap3Struct *)Item->Data;

	AddItem(Model, "Mapping Type",data->MappingType);
	AddItem(Model, "Frame Count",data->FrameCount);
	AddItem(Model, "Frame Rate",data->FrameRate);
}

void ChunkTableClass::List_W3D_CHUNK_MATERIAL3_DC_MAP(ChunkItem *Item, ChunkModel* Model)
{
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_MATERIAL3_DI_MAP(ChunkItem *Item, ChunkModel* Model)
{
	List_Subitems(Item,Model);
}

void ChunkTableClass::List_W3D_CHUNK_MATERIAL3_SC_MAP(ChunkItem *Item, ChunkModel* Model)
{
	List_Subitems(Item,Model);
}

void ChunkTableClass::List_W3D_CHUNK_MATERIAL3_SI_MAP(ChunkItem *Item, ChunkModel* Model)
{
	List_Subitems(Item,Model);
}

void ChunkTableClass::List_W3D_CHUNK_MESH_HEADER3(ChunkItem *Item, ChunkModel* Model) {

	struct W3dMeshHeader3Struct *data;
	data = (W3dMeshHeader3Struct *) Item->Data;
	

	char buf[64];
	sprintf(buf,"%d.%d",W3D_GET_MAJOR_VERSION(data->Version),W3D_GET_MINOR_VERSION(data->Version));
	AddItem(Model,"Version", buf);
	AddItem(Model,"MeshName", data->MeshName);
	AddItem(Model,"ContainerName", data->ContainerName);
	AddItem(Model,"Attributes",data->Attributes);
	switch(data->Attributes & W3D_MESH_FLAG_GEOMETRY_TYPE_MASK)
	{
		case W3D_MESH_FLAG_GEOMETRY_TYPE_NORMAL:
			AddItem(Model,"Attributes", "W3D_MESH_FLAG_GEOMETRY_TYPE_NORMAL_MESH","flag");
			break;
		case W3D_MESH_FLAG_GEOMETRY_TYPE_CAMERA_ALIGNED:
			AddItem(Model,"Attributes", "W3D_MESH_FLAG_GEOMETRY_TYPE_CAMERA_ALIGNED","flag");
			break;
		case W3D_MESH_FLAG_GEOMETRY_TYPE_SKIN:
			AddItem(Model,"Attributes", "W3D_MESH_FLAG_GEOMETRY_TYPE_SKIN","flag");
			break;
		case W3D_MESH_FLAG_GEOMETRY_TYPE_AABOX:
			AddItem(Model,"Attributes", "W3D_MESH_FLAG_GEOMETRY_TYPE_AABOX","flag");
			break;
		case W3D_MESH_FLAG_GEOMETRY_TYPE_OBBOX:
			AddItem(Model,"Attributes", "W3D_MESH_FLAG_GEOMETRY_TYPE_OBBOX","flag");
			break;
	};
	
	if (data->Attributes & W3D_MESH_FLAG_COLLISION_TYPE_PHYSICAL)	AddItem(Model,"Attributes", "W3D_MESH_FLAG_COLLISION_TYPE_PHYSICAL","flag");
	if (data->Attributes & W3D_MESH_FLAG_COLLISION_TYPE_PROJECTILE)	AddItem(Model,"Attributes", "W3D_MESH_FLAG_COLLISION_TYPE_PROJECTILE","flag");
	if (data->Attributes & W3D_MESH_FLAG_COLLISION_TYPE_VIS)	AddItem(Model,"Attributes", "W3D_MESH_FLAG_COLLISION_TYPE_VIS","flag");
	if (data->Attributes & W3D_MESH_FLAG_COLLISION_TYPE_CAMERA)	AddItem(Model,"Attributes", "W3D_MESH_FLAG_COLLISION_TYPE_CAMERA","flag");
	if (data->Attributes & W3D_MESH_FLAG_COLLISION_TYPE_VEHICLE)	AddItem(Model,"Attributes", "W3D_MESH_FLAG_COLLISION_TYPE_VEHICLE","flag");

	if (data->Attributes & W3D_MESH_FLAG_HIDDEN) AddItem(Model,"Attributes", "W3D_MESH_FLAG_HIDDEN","flag");
	if (data->Attributes & W3D_MESH_FLAG_TWO_SIDED) AddItem(Model,"Attributes", "W3D_MESH_FLAG_TWO_SIDED","flag");
	if (data->Attributes & W3D_MESH_FLAG_CAST_SHADOW) AddItem(Model,"Attributes", "W3D_MESH_FLAG_CAST_SHADOW");
	if (data->Attributes & W3D_MESH_FLAG_SHATTERABLE) AddItem(Model,"Attributes", "W3D_MESH_FLAG_SHATTERABLE");
	if (data->Attributes & W3D_MESH_FLAG_NPATCHABLE) AddItem(Model,"Attributes", "W3D_MESH_FLAG_NPATCHABLE");

	if (data->Attributes & W3D_MESH_FLAG_PRELIT_UNLIT) AddItem(Model,"Attributes", "W3D_MESH_FLAG_PRELIT_UNLIT","flag");
	if (data->Attributes & W3D_MESH_FLAG_PRELIT_VERTEX) AddItem(Model,"Attributes", "W3D_MESH_FLAG_PRELIT_VERTEX","flag");
	if (data->Attributes & W3D_MESH_FLAG_PRELIT_LIGHTMAP_MULTI_PASS) AddItem(Model,"Attributes", "W3D_MESH_FLAG_PRELIT_LIGHTMAP_MULTI_PASS","flag");
	if (data->Attributes & W3D_MESH_FLAG_PRELIT_LIGHTMAP_MULTI_TEXTURE) AddItem(Model,"Attributes", "W3D_MESH_FLAG_PRELIT_LIGHTMAP_MULTI_TEXTURE","flag");

	AddItem(Model,"NumTris", data->NumTris);
	AddItem(Model,"NumVertices", data->NumVertices);
	AddItem(Model,"NumMaterials", data->NumMaterials);
	AddItem(Model,"NumDamageStages", data->NumDamageStages);
	
	if (data->SortLevel == SORT_LEVEL_NONE) {
		AddItem(Model, "SortLevel", "NONE");
	} else {
		AddItem(Model, "SortLevel", (uint8)data->SortLevel);
	}

	if ((data->Attributes & W3D_MESH_FLAG_PRELIT_MASK) != 0x0) {
		if (data->PrelitVersion != 0) {
			sprintf (buf, "%d.%d", W3D_GET_MAJOR_VERSION (data->PrelitVersion), W3D_GET_MINOR_VERSION (data->PrelitVersion));
		} else {
			sprintf (buf, "UNKNOWN");
		}
	} else {	  
		sprintf (buf, "N/A");
	}
	AddItem(Model, "PrelitVersion", buf);

	AddItem(Model, "FutureCounts", data->FutureCounts, 1);
	AddItem(Model, "VertexChannels", data->VertexChannels);
	if (data->VertexChannels & W3D_VERTEX_CHANNEL_LOCATION) AddItem(Model,"VertexChannels","W3D_VERTEX_CHANNEL_LOCATION","flag");
	if (data->VertexChannels & W3D_VERTEX_CHANNEL_NORMAL) AddItem(Model,"VertexChannels","W3D_VERTEX_CHANNEL_NORMAL","flag");
	if (data->VertexChannels & W3D_VERTEX_CHANNEL_TEXCOORD) AddItem(Model,"VertexChannels","W3D_VERTEX_CHANNEL_TEXCOORD","flag");
	if (data->VertexChannels & W3D_VERTEX_CHANNEL_COLOR) AddItem(Model,"VertexChannels","W3D_VERTEX_CHANNEL_COLOR","flag");
	if (data->VertexChannels & W3D_VERTEX_CHANNEL_BONEID) AddItem(Model,"VertexChannels","W3D_VERTEX_CHANNEL_BONEID","flag");
	AddItem(Model,"FaceChannels", data->FaceChannels);
	if (data->FaceChannels & W3D_FACE_CHANNEL_FACE) AddItem(Model,"FaceChannels","W3D_FACE_CHANNEL_FACE","flag");
	AddItem(Model,"Min", &data->Min);
	AddItem(Model,"Max", &data->Max);
	AddItem(Model,"SphCenter", &data->SphCenter);
	AddItem(Model,"SphRadius", data->SphRadius);
}

void ChunkTableClass::List_W3D_CHUNK_TRIANGLES(ChunkItem *Item, ChunkModel* Model) {
	struct W3dTriStruct *data;
	data = (W3dTriStruct *) Item->Data;
	
	int counter = 0;
	void *max = (char *) Item->Data + Item->Length;
	
	char buf[256];
	while(data < max) {

		sprintf(buf, "Triangle[%d].VertexIndices", counter);
		AddItem(Model,buf, data->Vindex, 3);

		sprintf(buf, "Triangle[%d].Attributes", counter);
		AddItem(Model,buf, data->Attributes);

		sprintf(buf, "Triangle[%d].Normal", counter);
		AddItem(Model,buf, &data->Normal);

		sprintf(buf, "Triangle[%d].Dist", counter);
		AddItem(Model,buf, data->Dist);

		counter++;
		data++;
	}

}

void ChunkTableClass::List_W3D_CHUNK_PER_TRI_MATERIALS(ChunkItem * Item,ChunkModel* Model)
{
	unsigned short *data;
	data = (unsigned short *) Item->Data;
	
	int counter = 0;
	void *max = (char *) Item->Data + Item->Length;
	
	char buf[256];

	while(data < max) {
		sprintf(buf, "Triangle[%d].MaterialIdx", counter);
		AddItem(Model,buf, *data);
		counter++;
		data++;
	}
}


void	ChunkTableClass::List_W3D_CHUNK_VERTEX_SHADE_INDICES(ChunkItem * Item,ChunkModel* Model)
{
	uint32 * data = (uint32 *) Item->Data;
	
	int counter = 0;
	void *max = (char *) Item->Data + Item->Length;
	
	char buf[256];
	while(data < max) {
		sprintf(buf, "Index[%d]", counter);
		AddItem(Model, buf, *data);
		counter++;
		data++;
	}
}

void	ChunkTableClass::List_W3D_CHUNK_MATERIAL_INFO(ChunkItem * Item,ChunkModel* Model)
{
	W3dMaterialInfoStruct * matinfo = (W3dMaterialInfoStruct *)Item->Data;
	

	AddItem(Model,"PassCount", matinfo->PassCount);
	AddItem(Model,"VertexMaterialCount", matinfo->VertexMaterialCount);
	AddItem(Model,"ShaderCount", matinfo->ShaderCount);
	AddItem(Model,"TextureCount",matinfo->TextureCount);
}

void ChunkTableClass::List_W3D_CHUNK_VERTEX_MATERIALS(ChunkItem *Item,ChunkModel* Model)
{
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_VERTEX_MATERIAL(ChunkItem *Item,ChunkModel* Model)
{
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_VERTEX_MATERIAL_NAME(ChunkItem * Item,ChunkModel* Model)
{
	
	char * data = (char *)Item->Data;
	AddItem(Model,"Material Name:", data);
}

void	ChunkTableClass::List_W3D_CHUNK_VERTEX_MATERIAL_INFO(ChunkItem * Item,ChunkModel* Model)
{
	struct W3dVertexMaterialStruct *data = (W3dVertexMaterialStruct *)Item->Data;
	

	if (data->Attributes & W3DVERTMAT_USE_DEPTH_CUE)				AddItem(Model, "Material.Attributes", "W3DVERTMAT_USE_DEPTH_CUE", "flag");
	if (data->Attributes & W3DVERTMAT_ARGB_EMISSIVE_ONLY)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_ARGB_EMISSIVE_ONLY", "flag");
	if (data->Attributes & W3DVERTMAT_COPY_SPECULAR_TO_DIFFUSE)	AddItem(Model, "Material.Attributes", "W3DVERTMAT_COPY_SPECULAR_TO_DIFFUSE", "flag");
	if (data->Attributes & W3DVERTMAT_DEPTH_CUE_TO_ALPHA)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_DEPTH_CUE_TO_ALPHA", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_UV)						AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_UV", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_ENVIRONMENT)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_ENVIRONMENT", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_CHEAP_ENVIRONMENT)	AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_CHEAP_ENVIRONMENT", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_SCREEN)					AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_SCREEN", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_LINEAR_OFFSET)		AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_LINEAR_OFFSET", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_SILHOUETTE)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_SILHOUETTE", "flag");

	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_SCALE)							AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_SCALE", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_GRID)							AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_GRID", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_ROTATE)							AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_ROTATE", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_SINE_LINEAR_OFFSET)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_SINE_LINEAR_OFFSET", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_STEP_LINEAR_OFFSET)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_STEP_LINEAR_OFFSET", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_ZIGZAG_LINEAR_OFFSET)		AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_ZIGZAG_LINEAR_OFFSET", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_WS_CLASSIC_ENV)				AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_WS_CLASSIC_ENV", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_WS_ENVIRONMENT)				AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_WS_ENVIRONMENT", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_GRID_CLASSIC_ENV)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_GRID_CLASSIC_ENV", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_GRID_ENVIRONMENT)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_GRID_ENVIRONMENT", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_RANDOM)							AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_RANDOM", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_BUMPENV)						AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_BUMPENV", "flag");

	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_UV)						AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_UV", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_ENVIRONMENT)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_ENVIRONMENT", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_CHEAP_ENVIRONMENT)	AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_CHEAP_ENVIRONMENT", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_SCREEN)					AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_SCREEN", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_LINEAR_OFFSET)		AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_LINEAR_OFFSET", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_SILHOUETTE)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_SILHOUETTE", "flag");

	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_SCALE)							AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_SCALE", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_GRID)							AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_GRID", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_ROTATE)							AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_ROTATE", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_SINE_LINEAR_OFFSET)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_SINE_LINEAR_OFFSET", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_STEP_LINEAR_OFFSET)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_STEP_LINEAR_OFFSET", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_ZIGZAG_LINEAR_OFFSET)		AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_ZIGZAG_LINEAR_OFFSET", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_WS_CLASSIC_ENV)				AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_WS_CLASSIC_ENV", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_WS_ENVIRONMENT)				AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_WS_ENVIRONMENT", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_GRID_CLASSIC_ENV)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_GRID_CLASSIC_ENV", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_GRID_ENVIRONMENT)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_GRID_ENVIRONMENT", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_RANDOM)							AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_RANDOM", "flag");
	if ((data->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_BUMPENV)						AddItem(Model, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_BUMPENV", "flag");

	if (data->Attributes & W3DVERTMAT_PSX_MASK) {
		if (data->Attributes & W3DVERTMAT_PSX_NO_RT_LIGHTING) {
			AddItem(Model, "Material.Attributes", "W3DVERTMAT_PSX_NO_RT_LIGHTING", "flag");
		}
		else {
			if ((data->Attributes & W3DVERTMAT_PSX_TRANS_MASK) == W3DVERTMAT_PSX_TRANS_NONE)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_PSX_TRANS_NONE", "flag");
			if ((data->Attributes & W3DVERTMAT_PSX_TRANS_MASK) == W3DVERTMAT_PSX_TRANS_100)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_PSX_TRANS_100", "flag");
			if ((data->Attributes & W3DVERTMAT_PSX_TRANS_MASK) == W3DVERTMAT_PSX_TRANS_50)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_PSX_TRANS_50", "flag");
			if ((data->Attributes & W3DVERTMAT_PSX_TRANS_MASK) == W3DVERTMAT_PSX_TRANS_25)			AddItem(Model, "Material.Attributes", "W3DVERTMAT_PSX_TRANS_25", "flag");
			if ((data->Attributes & W3DVERTMAT_PSX_TRANS_MASK) == W3DVERTMAT_PSX_TRANS_MINUS_100)	AddItem(Model, "Material.Attributes", "W3DVERTMAT_PSX_TRANS_MINUS_100", "flag");
		}
	}
 
	AddItem(Model, "Material.Attributes", data->Attributes);
	AddItem(Model, "Material.Ambient", &(data->Ambient));
	AddItem(Model, "Material.Diffuse", &(data->Diffuse));
	AddItem(Model, "Material.Specular", &(data->Specular));
	AddItem(Model, "Material.Emissive", &(data->Emissive));
	AddItem(Model, "Material.Shininess", data->Shininess);
	AddItem(Model, "Material.Opacity", data->Opacity);
	AddItem(Model, "Material.Translucency", data->Translucency);

}

void ChunkTableClass::List_W3D_CHUNK_VERTEX_MAPPER_ARGS0(ChunkItem *Item, ChunkModel* Model)
{
	
	char * data = (char *)Item->Data;
	AddItem(Model, "Stage0 Mapper Args:", data);
}

void ChunkTableClass::List_W3D_CHUNK_VERTEX_MAPPER_ARGS1(ChunkItem *Item, ChunkModel* Model)
{
	
	char * data = (char *)Item->Data;
	AddItem(Model, "Stage1 Mapper Args:", data);
}

void	ChunkTableClass::List_W3D_CHUNK_SHADERS(ChunkItem * Item,ChunkModel* Model)
{
	struct W3dShaderStruct *shader = (W3dShaderStruct *)Item->Data;
	void *max = (char *)Item->Data + Item->Length;
	char label[256];
    int counter = 0;

	while(shader < max) {
		sprintf(label,"shader[%d]",counter);
		AddItem(Model,label,shader);
		counter++;
		shader++;
	}
}

void	ChunkTableClass::List_W3D_CHUNK_PS2_SHADERS(ChunkItem * Item,ChunkModel* Model)
{
	struct W3dPS2ShaderStruct *shader = (W3dPS2ShaderStruct *)Item->Data;
	
	void *max = (char *)Item->Data + Item->Length;
	int counter = 0;
	char label[256];

	while(shader < max) {
		sprintf(label,"shader[%d]",counter);
		AddItem(Model,label,shader);
		counter++;
		shader++;
	}
}

void ChunkTableClass::List_W3D_CHUNK_TEXTURES(ChunkItem * Item,ChunkModel* Model)
{
	List_Subitems(Item,Model);
}

void ChunkTableClass::List_W3D_CHUNK_TEXTURE(ChunkItem * Item,ChunkModel* Model)
{
	List_Subitems(Item,Model);
}

void ChunkTableClass::List_W3D_CHUNK_TEXTURE_NAME(ChunkItem * Item,ChunkModel* Model)
{
	char * data = (char *)Item->Data;
	AddItem(Model,"Texture Name:", data);
}

void ChunkTableClass::List_W3D_CHUNK_TEXTURE_INFO(ChunkItem * Item,ChunkModel* Model)
{
	struct W3dTextureInfoStruct *data = (W3dTextureInfoStruct *)Item->Data;
	

	AddItem(Model, "Texture.Attributes", data->Attributes);
	
	if (data->Attributes & W3DTEXTURE_PUBLISH)		AddItem(Model,"Attributes", "W3DTEXTURE_PUBLISH","flag");
	if (data->Attributes & W3DTEXTURE_NO_LOD)			AddItem(Model,"Attributes", "W3DTEXTURE_NO_LOD","flag");
	if (data->Attributes & W3DTEXTURE_CLAMP_U)		AddItem(Model,"Attributes", "W3DTEXTURE_CLAMP_U","flag");
	if (data->Attributes & W3DTEXTURE_CLAMP_V)		AddItem(Model,"Attributes", "W3DTEXTURE_CLAMP_V","flag");
	if (data->Attributes & W3DTEXTURE_ALPHA_BITMAP)	AddItem(Model,"Attributes", "W3DTEXTURE_ALPHA_BITMAP","flag");

	AddItem(Model, "Texture.FrameCount", data->FrameCount);
	AddItem(Model, "Texture.FrameRate", data->FrameRate);
}

void	ChunkTableClass::List_W3D_CHUNK_MATERIAL_PASS(ChunkItem * Item,ChunkModel* Model)
{
	List_Subitems(Item,Model);
}

void	ChunkTableClass::List_W3D_CHUNK_VERTEX_MATERIAL_IDS(ChunkItem * Item,ChunkModel* Model)
{
	uint32 *data = (uint32 *)Item->Data;
	void *max = (char *) Item->Data + Item->Length;
	int counter = 0;
	char buf[256];

	while(data < max) {
	
		sprintf(buf, "Vertex[%d] Vertex Material Index", counter);
		AddItem(Model,buf, *data);

		counter++;
		data++;
	}
}

void	ChunkTableClass::List_W3D_CHUNK_SHADER_IDS(ChunkItem * Item,ChunkModel* Model)
{
	
	
	uint32 *data = (uint32 *)Item->Data;
	void *max = (char *) Item->Data + Item->Length;
	int counter = 0;
	char buf[256];

	while(data < max) {
	
		sprintf(buf, "Face[%d] Shader Index", counter);
		AddItem(Model, buf, *data);

		counter++;
		data++;
	}
}

void	ChunkTableClass::List_W3D_CHUNK_DCG(ChunkItem * Item,ChunkModel* Model)
{
	W3dRGBAStruct *data = (W3dRGBAStruct *) Item->Data;
	
	int counter = 0;
	void *max = (char *) Item->Data + Item->Length;
	
	char buf[256];

	while(data < max) {
	
		sprintf(buf, "Vertex[%d].DCG", counter);
		AddItem(Model,buf, data);

		counter++;
		data++;
	}
}

void	ChunkTableClass::List_W3D_CHUNK_DIG(ChunkItem * Item,ChunkModel* Model)
{
	W3dRGBStruct *data = (W3dRGBStruct *)Item->Data;
	
	int counter = 0;
	void *max = (char *) Item->Data + Item->Length;
	
	char buf[256];

	while(data < max) {
	
		sprintf(buf, "Vertex[%d].DIG", counter);
		AddItem(Model,buf, data);

		counter++;
		data++;
	}
}

void	ChunkTableClass::List_W3D_CHUNK_SCG(ChunkItem * Item,ChunkModel* Model)
{
	W3dRGBStruct *data = (W3dRGBStruct *)Item->Data;
	
	int counter = 0;
	void *max = (char *) Item->Data + Item->Length;
	
	char buf[256];

	while(data < max) {
	
		sprintf(buf, "Vertex[%d].SCG", counter);
		AddItem(Model,buf, data);

		counter++;
		data++;
	}
}
		
void	ChunkTableClass::List_W3D_CHUNK_TEXTURE_STAGE(ChunkItem * Item,ChunkModel* Model)
{
	List_Subitems(Item,Model);
}

void	ChunkTableClass::List_W3D_CHUNK_TEXTURE_IDS(ChunkItem * Item,ChunkModel* Model)
{	
	int counter = 0;
	uint32 *data = (uint32 *)Item->Data;
	void *max = (char *) Item->Data + Item->Length;
	char buf[256];

	while(data < max) {
	
		sprintf(buf, "Face[%d] Texture Index", counter);
		AddItem(Model, buf, *data);

		counter++;
		data++;
	}
}

void	ChunkTableClass::List_W3D_CHUNK_STAGE_TEXCOORDS(ChunkItem * Item,ChunkModel* Model)
{
	W3dTexCoordStruct *data = (W3dTexCoordStruct *)Item->Data;
	
	int counter = 0;
	void *max = (char *) Item->Data + Item->Length;
	
	char buf[256];

	while(data < max) {
	
		sprintf(buf, "Vertex[%d].UV", counter);
		AddItem(Model, buf, data);

		counter++;
		data++;
	}
}

void	ChunkTableClass::List_W3D_CHUNK_PER_FACE_TEXCOORD_IDS(ChunkItem * Item,ChunkModel* Model)
{
	int counter = 0;	
	Vector3i *data = (Vector3i *)Item->Data;
	void *max = (char *) Item->Data + Item->Length;
	char buf[256];

	while(data < max) {
	
		sprintf(buf, "Face[%d] UV Indices", counter);
		AddItem(Model, buf, data);

		counter++;
		data++;
	}
}

void ChunkTableClass::List_W3D_CHUNK_AABTREE(ChunkItem * Item,ChunkModel* Model)
{
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_AABTREE_HEADER(ChunkItem * Item,ChunkModel* Model)
{
	W3dMeshAABTreeHeader * data = (W3dMeshAABTreeHeader*)Item->Data;
	
	AddItem(Model, "NodeCount", data->NodeCount);
	AddItem(Model, "PolyCount", data->PolyCount);
}

void ChunkTableClass::List_W3D_CHUNK_AABTREE_POLYINDICES(ChunkItem * Item,ChunkModel* Model)
{
	uint32 *data = (uint32 *)Item->Data;
	void *max = (char *) Item->Data + Item->Length;
	int counter = 0;
	char buf[256];

	while(data < max) {
	
		sprintf(buf, "Polygon Index[%d]", counter);
		AddItem(Model, buf, *data);

		counter++;
		data++;
	}
}

void ChunkTableClass::List_W3D_CHUNK_AABTREE_NODES(ChunkItem * Item,ChunkModel* Model)
{
	W3dMeshAABTreeNode * data = (W3dMeshAABTreeNode *)Item->Data;
	
	int counter = 0;
	void *max = (char *)Item->Data + Item->Length;
	
	char buf[256];
	
	while(data < max) {

		sprintf(buf, "Node[%d].Min", counter);
		AddItem(Model,buf, &data->Min);

		sprintf(buf, "Node[%d].Max", counter);
		AddItem(Model,buf, &data->Max);

		if (data->FrontOrPoly0 & 0x80000000) {
			sprintf(buf, "Node[%d].Poly0",counter);
			AddItem(Model, buf, data->FrontOrPoly0 & 0x7FFFFFFF);
			sprintf(buf, "Node[%d].PolyCount",counter);
			AddItem(Model, buf, data->BackOrPolyCount);
		} else {
			sprintf(buf, "Node[%d].Front",counter);
			AddItem(Model, buf, data->FrontOrPoly0);
			sprintf(buf, "Node[%d].Back",counter);
			AddItem(Model, buf, data->BackOrPolyCount);
		}

		counter++;
		data++;
	}
}

void ChunkTableClass::List_W3D_CHUNK_HIERARCHY(ChunkItem *Item, ChunkModel* Model) {
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_HIERARCHY_HEADER(ChunkItem *Item, ChunkModel* Model) {
	struct W3dHierarchyStruct *data;
	data = (W3dHierarchyStruct *) Item->Data;

	
	char buf[64];
	sprintf(buf,"%d.%d",W3D_GET_MAJOR_VERSION(data->Version),W3D_GET_MINOR_VERSION(data->Version));
	AddItem(Model,"Version", buf);
	AddItem(Model, "Name", data->Name);
	AddItem(Model, "NumPivots", data->NumPivots);
	AddItem(Model, "Center", &data->Center);
}
void ChunkTableClass::List_W3D_CHUNK_PIVOTS(ChunkItem *Item, ChunkModel* Model) {
	struct W3dPivotStruct *data;
	data = (W3dPivotStruct *) Item->Data;
	
	int counter = 0;
	void *max = (char *) Item->Data + Item->Length;
	
	char buf[256];
	while(data < max) {

		sprintf(buf, "Pivot[%d].Name", counter);
		AddItem(Model,buf, data->Name);

		sprintf(buf, "Pivot[%d].ParentIdx", counter);
		AddItem(Model,buf, data->ParentIdx);

		sprintf(buf, "Pivot[%d].Translation", counter);
		AddItem(Model,buf, &data->Translation);

		sprintf(buf, "Pivot[%d].EulerAngles", counter);
		AddItem(Model,buf, &data->EulerAngles);

		sprintf(buf, "Pivot[%d].Rotation", counter);
		AddItem(Model,buf, &data->Rotation);

		counter++;
		data++;
	}
}

void ChunkTableClass::List_W3D_CHUNK_PIVOT_FIXUPS(ChunkItem *Item, ChunkModel* Model) {
	W3dPivotFixupStruct *data = (W3dPivotFixupStruct *) Item->Data;
	
	
	int pivot_counter = 0;

	while ((char*)data < (char*)Item->Data + Item->Length) {
		char tmp[256];
		for (int i=0;i<4;i++) {
			sprintf(tmp,"Transform %d, Row[%d]", pivot_counter,i);
			AddItem(Model, tmp, data->TM[i], 3);
		}
		data++;
		pivot_counter++;
	}
}

void ChunkTableClass::List_W3D_CHUNK_ANIMATION(ChunkItem *Item, ChunkModel* Model) {
	List_Subitems(Item, Model);
}
void ChunkTableClass::List_W3D_CHUNK_ANIMATION_HEADER(ChunkItem *Item, ChunkModel* Model) {

	W3dAnimHeaderStruct *data = (W3dAnimHeaderStruct *) Item->Data;
	
	
	char buf[64];
	sprintf(buf,"%d.%d",W3D_GET_MAJOR_VERSION(data->Version),W3D_GET_MINOR_VERSION(data->Version));
	AddItem(Model,"Version", buf);
	AddItem(Model, "Name", data->Name);
	AddItem(Model, "HierarchyName", data->HierarchyName);
	AddItem(Model, "NumFrames", data->NumFrames);
	AddItem(Model, "FrameRate", data->FrameRate);
}

void ChunkTableClass::List_W3D_CHUNK_ANIMATION_CHANNEL(ChunkItem *Item, ChunkModel* Model) 
{
	static const char * _chntypes[] = {
		"X Translation",
		"Y Translation",
		"Z Translation",
		"X Rotation",
		"Y Rotation",
		"Z Rotation",
		"Quaternion"
	};


	W3dAnimChannelStruct *data = (W3dAnimChannelStruct *) Item->Data;
	
	AddItem(Model, "FirstFrame", data->FirstFrame);
	AddItem(Model, "LastFrame", data->LastFrame);

	if ((data->Flags >= ANIM_CHANNEL_X)&&(data->Flags <= ANIM_CHANNEL_Q)) {
		AddItem(Model, "ChannelType",_chntypes[data->Flags]);
	} else {
		AddItem(Model, "ChannelType",data->Flags);
	}

	AddItem(Model, "Pivot", data->Pivot);
	AddItem(Model, "VectorLen", data->VectorLen);

	for (int frameidx=0; frameidx <= data->LastFrame - data->FirstFrame; frameidx++) {
		for (int vidx = 0; vidx < data->VectorLen; vidx++) {
            char namebuf[256];
            snprintf(namebuf, 256, "Data[%d][%d]", frameidx + data->FirstFrame, vidx);
			AddItem(Model, namebuf, data->Data[frameidx * data->VectorLen + vidx]);
		}
	}
}

void ChunkTableClass::List_W3D_CHUNK_BIT_CHANNEL(ChunkItem *Item, ChunkModel* Model) 
{
	static const char * _chntypes[] = 
	{
		"Visibility",
	};


	W3dBitChannelStruct *data = (W3dBitChannelStruct *) Item->Data;
	
	unsigned char * bits = &(data->Data[0]);

	AddItem(Model, "FirstFrame", data->FirstFrame);
	AddItem(Model, "LastFrame", data->LastFrame);

	if ((data->Flags >= BIT_CHANNEL_VIS)&&(data->Flags <= BIT_CHANNEL_VIS)) {
		AddItem(Model, "ChannelType",_chntypes[data->Flags]);
	} else {
		AddItem(Model, "ChannelType",data->Flags);
	}

	AddItem(Model, "Pivot", data->Pivot);
	AddItem(Model, "Default Value", data->DefaultVal);

	for (int frameidx=0; frameidx <= data->LastFrame - data->FirstFrame; frameidx++) {
        char namebuf[256];
        snprintf(namebuf, 256, "Data[%d]", frameidx + data->FirstFrame);
		AddItem(Model, namebuf, (uint8)Get_Bit(bits,frameidx));
	}
}

void ChunkTableClass::List_W3D_CHUNK_HMODEL(ChunkItem *Item, ChunkModel* Model) {
	List_Subitems(Item, Model);
}
void ChunkTableClass::List_W3D_CHUNK_HMODEL_HEADER(ChunkItem *Item, ChunkModel* Model) {
	W3dHModelHeaderStruct *data = (W3dHModelHeaderStruct *) Item->Data;
	
	
	char buf[64];
	sprintf(buf,"%d.%d",W3D_GET_MAJOR_VERSION(data->Version),W3D_GET_MINOR_VERSION(data->Version));
	AddItem(Model,"Version", buf);
	AddItem(Model, "Name", data->Name);
	AddItem(Model, "HierarchyName", data->HierarchyName);
	AddItem(Model, "NumConnections", data->NumConnections);
}
void ChunkTableClass::List_W3D_CHUNK_NODE(ChunkItem *Item, ChunkModel* Model) {
	W3dHModelNodeStruct *data = (W3dHModelNodeStruct *) Item->Data;
	
	
	AddItem(Model, "RenderObjName", data->RenderObjName);
	AddItem(Model, "PivotIdx", data->PivotIdx);
}

void ChunkTableClass::List_W3D_CHUNK_COLLISION_NODE(ChunkItem *Item, ChunkModel* Model) {
	W3dHModelNodeStruct *data = (W3dHModelNodeStruct *) Item->Data;
	
	
	AddItem(Model, "CollisionMeshName", data->RenderObjName);
	AddItem(Model, "PivotIdx", data->PivotIdx);
}

void ChunkTableClass::List_W3D_CHUNK_SKIN_NODE(ChunkItem *Item, ChunkModel* Model) {
	W3dHModelNodeStruct *data = (W3dHModelNodeStruct *) Item->Data;
	
	
	AddItem(Model, "SkinMeshName", data->RenderObjName);
	AddItem(Model, "PivotIdx", data->PivotIdx);
}

void ChunkTableClass::List_W3D_CHUNK_HMODEL_AUX_DATA(ChunkItem *Item, ChunkModel* Model) {

	W3dHModelAuxDataStruct *data = (W3dHModelAuxDataStruct *) Item->Data;
	
	
	AddItem(Model, "Attributes", data->Attributes);
	AddItem(Model, "MeshCount", data->MeshCount);
	AddItem(Model, "CollisionCount", data->CollisionCount);
	AddItem(Model, "SkinCount", data->SkinCount);
	AddItem(Model, "FutureCounts", data->FutureCounts, 8);
	AddItem(Model, "LODMin", data->LODMin);
	AddItem(Model, "LODMax", data->LODMax);
	AddItem(Model, "FutureUse", data->FutureUse, 32);
}

void ChunkTableClass::List_W3D_CHUNK_SHADOW_NODE(ChunkItem *Item, ChunkModel* Model) {
	W3dHModelNodeStruct *data = (W3dHModelNodeStruct *) Item->Data;
	
	
	AddItem(Model, "ShadowMeshName", data->RenderObjName);
	AddItem(Model, "PivotIdx", data->PivotIdx);
}

void ChunkTableClass::List_W3D_CHUNK_LODMODEL(ChunkItem *Item, ChunkModel* Model)
{
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_LODMODEL_HEADER(ChunkItem *Item, ChunkModel* Model)
{
	W3dLODModelHeaderStruct *data = (W3dLODModelHeaderStruct *) Item->Data;
	

	char buf[64];
	sprintf(buf,"%d.%d",W3D_GET_MAJOR_VERSION(data->Version),W3D_GET_MINOR_VERSION(data->Version));
	AddItem(Model,"Version", buf);
	AddItem(Model, "Name", data->Name);
	AddItem(Model, "NumLODs", data->NumLODs);
}

void ChunkTableClass::List_W3D_CHUNK_LOD(ChunkItem * Item, ChunkModel* Model)
{
	W3dLODStruct *data = (W3dLODStruct *) Item->Data;
	
	AddItem(Model, "Render Object Name", data->RenderObjName);
	AddItem(Model, "LOD Min Distance", data->LODMin);
	AddItem(Model, "LOD Max Distance", data->LODMax);
}

void ChunkTableClass::List_W3D_CHUNK_COLLECTION(ChunkItem * Item, ChunkModel* Model)
{
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_COLLECTION_HEADER(ChunkItem * Item, ChunkModel* Model)
{
	W3dCollectionHeaderStruct *data = (W3dCollectionHeaderStruct *) Item->Data;
	

	char buf[64];
	sprintf(buf,"%d.%d",W3D_GET_MAJOR_VERSION(data->Version),W3D_GET_MINOR_VERSION(data->Version));
	AddItem(Model,"Version", buf);
	AddItem(Model, "Name", data->Name);
	AddItem(Model, "RenderObjectCount", data->RenderObjectCount);
}

void ChunkTableClass::List_W3D_CHUNK_COLLECTION_OBJ_NAME(ChunkItem * Item, ChunkModel* Model)
{
	char * name = (char *)Item->Data;
	
	AddItem(Model, "Render Object Name", name);	
}

void ChunkTableClass::List_W3D_CHUNK_PLACEHOLDER(ChunkItem * Item, ChunkModel* Model)
{
	W3dPlaceholderStruct * data = (W3dPlaceholderStruct *)(Item->Data);
	

	AddItemVersion(Model,data->version);
	AddItem(Model,"Transform",&(data->transform[0][0]),3);
	AddItem(Model,"Transform",&(data->transform[1][0]),3);
	AddItem(Model,"Transform",&(data->transform[2][0]),3);
	AddItem(Model,"Transform",&(data->transform[3][0]),3);
	AddItem(Model,"Name",(char *)(data + 1));
}

void ChunkTableClass::List_W3D_CHUNK_TRANSFORM_NODE(ChunkItem * Item, ChunkModel* Model)
{
	W3dTransformNodeStruct * data = (W3dTransformNodeStruct *)(Item->Data);
	

	AddItemVersion(Model,data->version);
	AddItem(Model,"Transform",&(data->transform[0][0]),3);
	AddItem(Model,"Transform",&(data->transform[1][0]),3);
	AddItem(Model,"Transform",&(data->transform[2][0]),3);
	AddItem(Model,"Transform",&(data->transform[3][0]),3);
	AddItem(Model,"Name",(char *)(data + 1));
}

void ChunkTableClass::List_W3D_CHUNK_POINTS(ChunkItem * Item, ChunkModel* Model)
{
	W3dVectorStruct *data;
	data = (W3dVectorStruct *) Item->Data;

	
	void *max = (char *) Item->Data + Item->Length;
	int counter = 0;
	char buf[256];

	while (data < max) {
		sprintf(buf, "Point[%d]", counter++);
		AddItem(Model,buf, data);
		data++;
	}
}

void ChunkTableClass::List_W3D_CHUNK_LIGHT(ChunkItem * Item, ChunkModel* Model)
{
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_LIGHT_INFO(ChunkItem * Item, ChunkModel* Model)
{
	W3dLightStruct * data = (W3dLightStruct *)Item->Data;
	

	if ((data->Attributes & W3D_LIGHT_ATTRIBUTE_TYPE_MASK) == W3D_LIGHT_ATTRIBUTE_POINT) AddItem(Model, "Attributes", "W3D_LIGHT_ATTRIBUTE_POINT");
	if ((data->Attributes & W3D_LIGHT_ATTRIBUTE_TYPE_MASK) == W3D_LIGHT_ATTRIBUTE_SPOT) AddItem(Model, "Attributes", "W3D_LIGHT_ATTRIBUTE_SPOT");
	if ((data->Attributes & W3D_LIGHT_ATTRIBUTE_TYPE_MASK) == W3D_LIGHT_ATTRIBUTE_DIRECTIONAL) AddItem(Model, "Attributes", "W3D_LIGHT_ATTRIBUTE_DIRECTIONAL");
	if (data->Attributes & W3D_LIGHT_ATTRIBUTE_CAST_SHADOWS)	AddItem(Model, "Attributes", "W3D_LIGHT_ATTRIBUTE_CAST_SHADOWS", "flag");
	AddItem(Model,"Ambient",&(data->Ambient),1);
	AddItem(Model,"Diffuse",&(data->Diffuse),1);
	AddItem(Model,"Specular",&(data->Specular),1);
	AddItem(Model,"Intensity",data->Intensity);
}

void ChunkTableClass::List_W3D_CHUNK_SPOT_LIGHT_INFO(ChunkItem * Item, ChunkModel* Model)
{
	W3dSpotLightStruct * data = (W3dSpotLightStruct*)Item->Data;
	
	AddItem(Model,"SpotDirection",&(data->SpotDirection));
	AddItem(Model,"SpotAngle",data->SpotAngle);
	AddItem(Model,"SpotExponent",data->SpotExponent);
}

void ChunkTableClass::List_W3D_CHUNK_NEAR_ATTENUATION(ChunkItem * Item, ChunkModel* Model)
{
	W3dLightAttenuationStruct * data = (W3dLightAttenuationStruct *)Item->Data;
	
	AddItem(Model,"Near Atten Start",data->Start);
	AddItem(Model,"Near Atten End",data->End);
}

void ChunkTableClass::List_W3D_CHUNK_FAR_ATTENUATION(ChunkItem * Item, ChunkModel* Model)
{
	W3dLightAttenuationStruct * data = (W3dLightAttenuationStruct *)Item->Data;
	
	AddItem(Model,"Far Atten Start",data->Start);
	AddItem(Model,"Far Atten End",data->End);
}

void ChunkTableClass::List_W3D_CHUNK_EMITTER(ChunkItem * Item, ChunkModel* Model)
{
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_EMITTER_HEADER(ChunkItem * Item, ChunkModel* Model)
{
	W3dEmitterHeaderStruct * data = (W3dEmitterHeaderStruct*)Item->Data;
	
	AddItem(Model,"Version",data->Version);
	AddItem(Model,"Name",data->Name);
}

void ChunkTableClass::List_W3D_CHUNK_EMITTER_USER_DATA(ChunkItem * Item, ChunkModel* Model)
{
	char * data = (char *)Item->Data;
	
	AddItem(Model,"User Data",data);
}

void ChunkTableClass::List_W3D_CHUNK_EMITTER_INFO(ChunkItem * Item, ChunkModel* Model)
{
	W3dEmitterInfoStruct * data = (W3dEmitterInfoStruct *)Item->Data;
	
	AddItem(Model,"Texture Name",data->TextureFilename);
	AddItem(Model,"StartSize", data->StartSize);
	AddItem(Model,"EndSize",data->EndSize);
	AddItem(Model,"Lifetime",data->Lifetime);
	AddItem(Model,"EmissionRate",data->EmissionRate);
	AddItem(Model,"MaxEmissions",data->MaxEmissions);
	AddItem(Model,"VelocityRandom",data->VelocityRandom);
	AddItem(Model,"PositionRandom",data->PositionRandom);
	AddItem(Model,"FadeTime",data->FadeTime);
	AddItem(Model,"Gravity",data->Gravity);
	AddItem(Model,"Elasticity",data->Elasticity);
	AddItem(Model,"Velocity",&(data->Velocity));
	AddItem(Model,"Acceleration",&(data->Acceleration));
	AddItem(Model,"StartColor",&(data->StartColor));
	AddItem(Model,"EndColor",&(data->EndColor));
}	

void ChunkTableClass::List_W3D_CHUNK_EMITTER_INFOV2(ChunkItem * Item, ChunkModel* Model)
{
	W3dEmitterInfoStructV2 * data = (W3dEmitterInfoStructV2 *)(Item->Data);
	
	AddItem(Model,"BurstSize",data->BurstSize);
	AddItem(Model,"CreationVolume.ClassID",data->CreationVolume.ClassID);
	AddItem(Model,"CreationVolume.Value1",data->CreationVolume.Value1);
	AddItem(Model,"CreationVolume.Value2",data->CreationVolume.Value2);
	AddItem(Model,"CreationVolume.Value3",data->CreationVolume.Value3);
	AddItem(Model,"VelRandom.ClassID",data->VelRandom.ClassID);
	AddItem(Model,"VelRandom.Value1",data->VelRandom.Value1);
	AddItem(Model,"VelRandom.Value2",data->VelRandom.Value2);
	AddItem(Model,"VelRandom.Value3",data->VelRandom.Value3);
	AddItem(Model,"OutwardVel",data->OutwardVel);
	AddItem(Model,"VelInherit",data->VelInherit);
	AddItem(Model,"Shader",&(data->Shader));
	AddItem(Model,"RenderMode",data->RenderMode);
}

void ChunkTableClass::List_W3D_CHUNK_EMITTER_PROPS(ChunkItem * Item, ChunkModel* Model)
{
	W3dEmitterPropertyStruct * data = (W3dEmitterPropertyStruct *)(Item->Data);
	
	AddItem(Model,"ColorKeyframes",data->ColorKeyframes);
	AddItem(Model,"OpacityKeyframes",data->OpacityKeyframes);
	AddItem(Model,"SizeKeyframes",data->SizeKeyframes);
	AddItem(Model,"ColorRandom",&(data->ColorRandom));
	AddItem(Model,"OpacityRandom",data->OpacityRandom);
	AddItem(Model,"SizeRandom",data->SizeRandom);
}

void ChunkTableClass::List_W3D_CHUNK_EMITTER_COLOR_KEYFRAME(ChunkItem * Item, ChunkModel* Model)
{
	W3dEmitterColorKeyframeStruct * data = (W3dEmitterColorKeyframeStruct *)(Item->Data);
	
	AddItem(Model,"Time",data->Time);
	AddItem(Model,"Color",&(data->Color));
}

void ChunkTableClass::List_W3D_CHUNK_EMITTER_OPACITY_KEYFRAME(ChunkItem * Item, ChunkModel* Model)
{
	W3dEmitterOpacityKeyframeStruct * data = (W3dEmitterOpacityKeyframeStruct *)(Item->Data);
	
	AddItem(Model,"Time",data->Time);
	AddItem(Model,"Opacity",data->Opacity);
}

void ChunkTableClass::List_W3D_CHUNK_EMITTER_SIZE_KEYFRAME(ChunkItem * Item, ChunkModel* Model)
{
	W3dEmitterSizeKeyframeStruct * data = (W3dEmitterSizeKeyframeStruct *)(Item->Data);
	
	AddItem(Model,"Time",data->Time);
	AddItem(Model,"Size",data->Size);
}

void ChunkTableClass::List_W3D_CHUNK_EMITTER_ROTATION_KEYFRAMES(ChunkItem * Item, ChunkModel* Model)
{
	W3dEmitterRotationHeaderStruct * header = (W3dEmitterRotationHeaderStruct*)(Item->Data);
	
	AddItem(Model,"KeyframeCount",header->KeyframeCount);
	AddItem(Model,"Random",header->Random);

	W3dEmitterRotationKeyframeStruct * key = (W3dEmitterRotationKeyframeStruct *)((char*)Item->Data + sizeof(W3dEmitterRotationHeaderStruct));
	char buf[256];
	for (unsigned int i=0; i<header->KeyframeCount+1; i++) {
		sprintf(buf,"Time[%d]",i);
		AddItem(Model,buf,key[i].Time);
		sprintf(buf,"Rotation[%d]",i);
		AddItem(Model,buf,key[i].Rotation);
	}
}

void ChunkTableClass::List_W3D_CHUNK_EMITTER_FRAME_KEYFRAMES(ChunkItem * Item, ChunkModel* Model)
{
	W3dEmitterFrameHeaderStruct * header = (W3dEmitterFrameHeaderStruct*)(Item->Data);
	
	AddItem(Model,"KeyframeCount",header->KeyframeCount);
	AddItem(Model,"Random",header->Random);

	W3dEmitterFrameKeyframeStruct * key = (W3dEmitterFrameKeyframeStruct *)((char *)Item->Data + sizeof(W3dEmitterFrameHeaderStruct));
	char buf[256];
	for (unsigned int i=0; i<header->KeyframeCount+1; i++) {
		sprintf(buf,"Time[%d]",i);
		AddItem(Model,buf,key[i].Time);
		sprintf(buf,"Frame[%d]",i);
		AddItem(Model,buf,key[i].Frame);
	}
}

void ChunkTableClass::List_W3D_CHUNK_EMITTER_BLUR_TIME_KEYFRAMES(ChunkItem * Item, ChunkModel* Model)
{
	W3dEmitterBlurTimeHeaderStruct * header = (W3dEmitterBlurTimeHeaderStruct*)(Item->Data);
	
	AddItem(Model,"KeyframeCount",header->KeyframeCount);
	AddItem(Model,"Random",header->Random);

	W3dEmitterBlurTimeKeyframeStruct * key = (W3dEmitterBlurTimeKeyframeStruct *)((char *)Item->Data + sizeof(W3dEmitterBlurTimeHeaderStruct));
	char buf[256];
	for (unsigned int i=0; i<header->KeyframeCount+1; i++) {
		sprintf(buf,"Time[%d]",i);
		AddItem(Model,buf,key[i].Time);
		sprintf(buf,"BlurTime[%d]",i);
		AddItem(Model,buf,key[i].BlurTime);
	}
}

void ChunkTableClass::List_W3D_CHUNK_AGGREGATE(ChunkItem * Item, ChunkModel* Model)
{
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_AGGREGATE_HEADER(ChunkItem * Item, ChunkModel* Model)
{
	W3dAggregateHeaderStruct * data = (W3dAggregateHeaderStruct*)Item->Data;
	
	AddItem(Model,"Version",data->Version);
	AddItem(Model,"Name",data->Name);
}

void ChunkTableClass::List_W3D_CHUNK_AGGREGATE_INFO(ChunkItem * Item, ChunkModel* Model)
{
	W3dAggregateInfoStruct * info = (W3dAggregateInfoStruct *)Item->Data;
	
	AddItem(Model,"BaseModelName",info->BaseModelName);
	AddItem(Model,"SubobjectCount",info->SubobjectCount);

	char label[256];
	W3dAggregateSubobjectStruct * defs = (W3dAggregateSubobjectStruct *)((char*)Item->Data + sizeof(W3dAggregateInfoStruct));
	
	for (unsigned int subobj=0; subobj<info->SubobjectCount; subobj++) {		
		sprintf(label,"SubObject[%d].SubobjectName",subobj);
		AddItem(Model,label,defs[subobj].SubobjectName);
		sprintf(label,"SubObject[%d].BoneName",subobj);
		AddItem(Model,label,defs[subobj].BoneName);

	}
}

void ChunkTableClass::List_W3D_CHUNK_TEXTURE_REPLACER_INFO(ChunkItem * Item, ChunkModel* Model)
{
	W3dTextureReplacerHeaderStruct * header = (W3dTextureReplacerHeaderStruct *)(Item->Data);
	
	AddItem(Model,"ReplacedTexturesCount",header->ReplacedTexturesCount);

	W3dTextureReplacerStruct * data = (W3dTextureReplacerStruct *)(header + 1);

	for (uint32 replaceidx=0; replaceidx<header->ReplacedTexturesCount; replaceidx++) {
		int pathidx = 0;
		char label[256];

		for (pathidx=0; pathidx<MESH_PATH_ENTRIES; pathidx++){
			sprintf(label,"Replacer[%d].MeshPath[%d]",replaceidx,pathidx);
			AddItem(Model,label,data->MeshPath[pathidx]);
		}

		for (pathidx=0; pathidx<MESH_PATH_ENTRIES; pathidx++){
			sprintf(label,"Replacer[%d].BonePath[%d]",replaceidx,pathidx);
			AddItem(Model,label,data->BonePath[pathidx]);
		}
		
		AddItem(Model,"OldTextureName",data->OldTextureName);
		AddItem(Model,"NewTextureName",data->NewTextureName);
		AddItem(Model,"TextureParams.Attributes", data->TextureParams.Attributes);
		AddItem(Model,"TextureParams.FrameCount", data->TextureParams.FrameCount);
		AddItem(Model,"TextureParams.FrameRate", data->TextureParams.FrameRate);
		data++;
	}
}

void ChunkTableClass::List_W3D_CHUNK_AGGREGATE_CLASS_INFO(ChunkItem * Item, ChunkModel* Model)
{
	W3dAggregateMiscInfo * data = (W3dAggregateMiscInfo *)(Item->Data);
	
	AddItem(Model,"OriginalClassID",data->OriginalClassID);
	AddItem(Model,"Flags",data->Flags);
}

void ChunkTableClass::List_W3D_CHUNK_HLOD(ChunkItem * Item, ChunkModel* Model)
{
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_HLOD_HEADER(ChunkItem * Item, ChunkModel* Model)
{
	W3dHLodHeaderStruct * header = (W3dHLodHeaderStruct *)Item->Data;
	
	
	AddItem(Model,"Version",header->Version);
	AddItem(Model,"LodCount", header->LodCount);
	AddItem(Model,"Name",header->Name);
	AddItem(Model,"HTree Name", header->HierarchyName);
}

void ChunkTableClass::List_W3D_CHUNK_HLOD_LOD_ARRAY(ChunkItem * Item, ChunkModel* Model)
{
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_HLOD_LOD_ARRAY_HEADER(ChunkItem * Item, ChunkModel* Model)
{
	W3dHLodArrayHeaderStruct * header = (W3dHLodArrayHeaderStruct *)Item->Data;
	

	AddItem(Model,"ModelCount",header->ModelCount);
	AddItem(Model,"MaxScreenSize",header->MaxScreenSize);
}

void ChunkTableClass::List_W3D_CHUNK_HLOD_SUB_OBJECT(ChunkItem * Item, ChunkModel* Model)
{
	W3dHLodSubObjectStruct * data = (W3dHLodSubObjectStruct *)Item->Data;
	
	AddItem(Model,"Name",data->Name);
	AddItem(Model,"BoneIndex",data->BoneIndex);
}

void ChunkTableClass::List_W3D_CHUNK_BOX(ChunkItem * Item, ChunkModel* Model)
{
	W3dBoxStruct * box = (W3dBoxStruct *)Item->Data;
	
	AddItem(Model,"Version",box->Version);
	AddItem(Model,"Attributes",box->Attributes);

	if (box->Attributes & W3D_BOX_ATTRIBUTE_ORIENTED) {
		AddItem(Model,"Attributes","W3D_BOX_ATTRIBUTE_ORIENTED","flag");
	}
	if (box->Attributes & W3D_BOX_ATTRIBUTE_ALIGNED) {
		AddItem(Model,"Attributes","W3D_BOX_ATTRIBUTE_ALIGNED","flag");
	}
	if (box->Attributes & W3D_BOX_ATTRIBTUE_COLLISION_TYPE_PHYSICAL) {
		AddItem(Model,"Attributes","W3D_BOX_ATTRIBTUE_COLLISION_TYPE_PHYSICAL","flag");
	}
	if (box->Attributes & W3D_BOX_ATTRIBTUE_COLLISION_TYPE_PROJECTILE) {
		AddItem(Model,"Attributes","W3D_BOX_ATTRIBTUE_COLLISION_TYPE_PROJECTILE","flag");
	}
	if (box->Attributes & W3D_BOX_ATTRIBTUE_COLLISION_TYPE_VIS) {
		AddItem(Model,"Attributes","W3D_BOX_ATTRIBTUE_COLLISION_TYPE_VIS","flag");
	}
	if (box->Attributes & W3D_BOX_ATTRIBTUE_COLLISION_TYPE_CAMERA) {
		AddItem(Model,"Attributes","W3D_BOX_ATTRIBTUE_COLLISION_TYPE_CAMERA","flag");
	}
	if (box->Attributes & W3D_BOX_ATTRIBTUE_COLLISION_TYPE_VEHICLE) {
		AddItem(Model,"Attributes","W3D_BOX_ATTRIBTUE_COLLISION_TYPE_VEHICLE","flag");
	}
	
	AddItem(Model,"Name",box->Name);
	AddItem(Model,"Color",&(box->Color));
	AddItem(Model,"Center",&(box->Center));
	AddItem(Model,"Extent",&(box->Extent));
}

void ChunkTableClass::List_W3D_CHUNK_NULL_OBJECT(ChunkItem * Item, ChunkModel* Model)
{
	W3dNullObjectStruct * null = (W3dNullObjectStruct *)Item->Data;
	
	AddItemVersion(Model,null->Version);
	AddItem(Model,"Attributes",null->Attributes);

	// No attributes are currently used

	AddItem(Model,"Name",null->Name);
}

void ChunkTableClass::List_W3D_CHUNK_PRELIT_UNLIT(ChunkItem *Item, ChunkModel* Model) {
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_PRELIT_VERTEX(ChunkItem *Item, ChunkModel* Model) {
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_PRELIT_LIGHTMAP_MULTI_PASS(ChunkItem *Item, ChunkModel* Model) {
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_PRELIT_LIGHTMAP_MULTI_TEXTURE(ChunkItem *Item, ChunkModel* Model) {
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_LIGHTSCAPE(ChunkItem *Item, ChunkModel* Model) {
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_LIGHTSCAPE_LIGHT(ChunkItem *Item, ChunkModel* Model) {
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_LIGHT_TRANSFORM(ChunkItem *Item, ChunkModel* Model) {

	W3dLightTransformStruct *data = (W3dLightTransformStruct*) (Item->Data);

	AddItem(Model, "Transform", &(data->Transform [0][0]), 4);
	AddItem(Model, "Transform", &(data->Transform [1][0]), 4);
	AddItem(Model, "Transform", &(data->Transform [2][0]), 4);
}

void ChunkTableClass::List_W3D_CHUNK_DAZZLE(ChunkItem *Item, ChunkModel* Model)
{
	List_Subitems(Item, Model);
}

void ChunkTableClass::List_W3D_CHUNK_DAZZLE_NAME(ChunkItem *Item, ChunkModel* Model)
{
	
	AddItem (Model, "Dazzle Name", (char *)(Item->Data));
}

void ChunkTableClass::List_W3D_CHUNK_DAZZLE_TYPENAME(ChunkItem *Item, ChunkModel* Model)
{
	AddItem(Model, "Dazzle Type Name", (char *)(Item->Data));
}

ChunkTableClass::ChunkTableClass() {

	NewType( W3D_CHUNK_MESH, "CHUNK_MESH", List_W3D_CHUNK_MESH, true);
	NewType( W3D_CHUNK_MESH_HEADER, "W3D_CHUNK_MESH_HEADER", List_W3D_CHUNK_MESH_HEADER);
	NewType( W3D_CHUNK_VERTICES, "W3D_CHUNK_VERTICES", List_W3D_CHUNK_VERTICES);
	NewType( W3D_CHUNK_VERTEX_NORMALS, "W3D_CHUNK_VERTEX_NORMALS", List_W3D_CHUNK_VERTEX_NORMALS);
	NewType( W3D_CHUNK_SURRENDER_NORMALS, "W3D_CHUNK_SURRENDER_NORMALS", List_W3D_CHUNK_SURRENDER_NORMALS);
	NewType( W3D_CHUNK_TEXCOORDS, "W3D_CHUNK_TEXCOORDS", List_W3D_CHUNK_TEXCOORDS);
	NewType( O_W3D_CHUNK_MATERIALS, "O_W3D_CHUNK_MATERIALS", List_O_W3D_CHUNK_MATERIALS);
	NewType( O_W3D_CHUNK_TRIANGLES, "O_W3D_CHUNK_TRIANGLES", List_O_W3D_CHUNK_TRIANGLES);
	NewType( O_W3D_CHUNK_QUADRANGLES, "O_W3D_CHUNK_QUADRANGLES", List_O_W3D_CHUNK_QUADRANGLES);
	NewType( O_W3D_CHUNK_SURRENDER_TRIANGLES, "O_W3D_CHUNK_SURRENDER_TRIANGLES", List_O_W3D_CHUNK_SURRENDER_TRIANGLES);
	NewType( O_W3D_CHUNK_POV_TRIANGLES, "O_W3D_CHUNK_POV_TRIANGLES", List_O_W3D_CHUNK_POV_TRIANGLES);
	NewType( O_W3D_CHUNK_POV_QUADRANGLES, "O_W3D_CHUNK_POV_QUADRANGLES", List_O_W3D_CHUNK_POV_QUADRANGLES);
	NewType( W3D_CHUNK_MESH_USER_TEXT, "W3D_CHUNK_MESH_USER_TEXT", List_W3D_CHUNK_MESH_USER_TEXT);
	NewType( W3D_CHUNK_VERTEX_COLORS, "W3D_CHUNK_VERTEX_COLORS", List_W3D_CHUNK_VERTEX_COLORS);
	NewType( W3D_CHUNK_VERTEX_INFLUENCES, "W3D_CHUNK_VERTEX_INFLUENCES", List_W3D_CHUNK_VERTEX_INFLUENCES);
	
	NewType( W3D_CHUNK_DAMAGE, "W3D_CHUNK_DAMAGE", List_W3D_CHUNK_DAMAGE, true);
	NewType( W3D_CHUNK_DAMAGE_HEADER, "W3D_CHUNK_DAMAGE_HEADER", List_W3D_CHUNK_DAMAGE_HEADER);
	NewType( W3D_CHUNK_DAMAGE_VERTICES, "W3D_CHUNK_DAMAGE_VERTICES", List_W3D_CHUNK_DAMAGE_VERTICES);
	NewType( W3D_CHUNK_DAMAGE_COLORS, "W3D_CHUNK_DAMAGE_COLORS", List_W3D_CHUNK_DAMAGE_COLORS);

	NewType( O_W3D_CHUNK_MATERIALS2, "O_W3D_CHUNK_MATERIALS2", List_O_W3D_CHUNK_MATERIALS2);

	NewType( W3D_CHUNK_MATERIALS3, "W3D_CHUNK_MATERIALS3", List_W3D_CHUNK_MATERIALS3, true);
	NewType( W3D_CHUNK_MATERIAL3, "W3D_CHUNK_MATERIAL3", List_W3D_CHUNK_MATERIAL3, true);
	NewType( W3D_CHUNK_MATERIAL3_NAME, "W3D_CHUNK_MATERIAL3_NAME", List_W3D_CHUNK_MATERIAL3_NAME);
	NewType( W3D_CHUNK_MATERIAL3_INFO, "W3D_CHUNK_MATERIAL3_INFO", List_W3D_CHUNK_MATERIAL3_INFO);
	NewType( W3D_CHUNK_MATERIAL3_DC_MAP, "W3D_CHUNK_MATERIAL3_DC_MAP", List_W3D_CHUNK_MATERIAL3_DC_MAP,true);
	NewType( W3D_CHUNK_MATERIAL3_DI_MAP, "W3D_CHUNK_MATERIAL3_DI_MAP", List_W3D_CHUNK_MATERIAL3_DI_MAP,true);
	NewType( W3D_CHUNK_MATERIAL3_SC_MAP, "W3D_CHUNK_MATERIAL3_SC_MAP", List_W3D_CHUNK_MATERIAL3_SC_MAP,true);
	NewType( W3D_CHUNK_MATERIAL3_SI_MAP, "W3D_CHUNK_MATERIAL3_SI_MAP", List_W3D_CHUNK_MATERIAL3_SI_MAP,true);
	NewType( W3D_CHUNK_MAP3_FILENAME, "W3D_CHUNK_MAP3_FILENAME", List_W3D_CHUNK_MAP3_FILENAME);
	NewType( W3D_CHUNK_MAP3_INFO, "W3D_CHUNK_MAP3_INFO", List_W3D_CHUNK_MAP3_INFO);

	NewType( W3D_CHUNK_MESH_HEADER3, "W3D_CHUNK_MESH_HEADER3",List_W3D_CHUNK_MESH_HEADER3);
	NewType( W3D_CHUNK_TRIANGLES, "W3D_CHUNK_TRIANGLES",List_W3D_CHUNK_TRIANGLES);
	NewType( W3D_CHUNK_PER_TRI_MATERIALS,"W3D_CHUNK_PER_TRI_MATERIALS",List_W3D_CHUNK_PER_TRI_MATERIALS);

	NewType( W3D_CHUNK_VERTEX_SHADE_INDICES, "W3D_CHUNK_VERTEX_SHADE_INDICES",List_W3D_CHUNK_VERTEX_SHADE_INDICES);
	NewType( W3D_CHUNK_MATERIAL_INFO,"W3D_CHUNK_MATERIAL_INFO",List_W3D_CHUNK_MATERIAL_INFO);
	NewType( W3D_CHUNK_SHADERS,"W3D_CHUNK_SHADERS",List_W3D_CHUNK_SHADERS);
	NewType( W3D_CHUNK_PS2_SHADERS,"W3D_CHUNK_PS2_SHADERS",List_W3D_CHUNK_PS2_SHADERS);

	NewType( W3D_CHUNK_VERTEX_MATERIALS, "W3D_CHUNK_VERTEX_MATERIALS",List_W3D_CHUNK_VERTEX_MATERIALS,true);
	NewType( W3D_CHUNK_VERTEX_MATERIAL, "W3D_CHUNK_VERTEX_MATERIAL",List_W3D_CHUNK_VERTEX_MATERIAL,true);
	NewType( W3D_CHUNK_VERTEX_MATERIAL_NAME, "W3D_CHUNK_VERTEX_MATERIAL_NAME",List_W3D_CHUNK_VERTEX_MATERIAL_NAME);
	NewType( W3D_CHUNK_VERTEX_MATERIAL_INFO, "W3D_CHUNK_VERTEX_MATERIAL_INFO",List_W3D_CHUNK_VERTEX_MATERIAL_INFO);
	NewType( W3D_CHUNK_VERTEX_MAPPER_ARGS0, "W3D_CHUNK_VERTEX_MAPPER_ARGS0",List_W3D_CHUNK_VERTEX_MAPPER_ARGS0);
	NewType( W3D_CHUNK_VERTEX_MAPPER_ARGS1, "W3D_CHUNK_VERTEX_MAPPER_ARGS1",List_W3D_CHUNK_VERTEX_MAPPER_ARGS1);

	NewType( W3D_CHUNK_TEXTURES, "W3D_CHUNK_TEXTURES", List_W3D_CHUNK_TEXTURES,true);
	NewType( W3D_CHUNK_TEXTURE, "W3D_CHUNK_TEXTURE", List_W3D_CHUNK_TEXTURE,true);
	NewType( W3D_CHUNK_TEXTURE_NAME, "W3D_CHUNK_TEXTURE_NAME", List_W3D_CHUNK_TEXTURE_NAME);
	NewType( W3D_CHUNK_TEXTURE_INFO, "W3D_CHUNK_TEXTURE_INFO", List_W3D_CHUNK_TEXTURE_INFO);
	
	NewType( W3D_CHUNK_MATERIAL_PASS, "W3D_CHUNK_MATERIAL_PASS", List_W3D_CHUNK_MATERIAL_PASS,true);
	NewType( W3D_CHUNK_VERTEX_MATERIAL_IDS, "W3D_CHUNK_VERTEX_MATERIAL_IDS", List_W3D_CHUNK_VERTEX_MATERIAL_IDS);
	NewType( W3D_CHUNK_SHADER_IDS, "W3D_CHUNK_SHADER_IDS", List_W3D_CHUNK_SHADER_IDS);
	NewType( W3D_CHUNK_DCG, "W3D_CHUNK_DCG", List_W3D_CHUNK_DCG);
	NewType( W3D_CHUNK_DIG, "W3D_CHUNK_DIG", List_W3D_CHUNK_DIG);
	NewType( W3D_CHUNK_SCG, "W3D_CHUNK_SCG", List_W3D_CHUNK_SCG);
	
	NewType( W3D_CHUNK_TEXTURE_STAGE, "W3D_CHUNK_TEXTURE_STAGE", List_W3D_CHUNK_TEXTURE_STAGE,true);
	NewType( W3D_CHUNK_TEXTURE_IDS, "W3D_CHUNK_TEXTURE_IDS", List_W3D_CHUNK_TEXTURE_IDS);
	NewType( W3D_CHUNK_STAGE_TEXCOORDS, "W3D_CHUNK_STAGE_TEXCOORDS", List_W3D_CHUNK_STAGE_TEXCOORDS);
	NewType( W3D_CHUNK_PER_FACE_TEXCOORD_IDS, "W3D_CHUNK_PER_FACE_TEXCOORD_IDS", List_W3D_CHUNK_PER_FACE_TEXCOORD_IDS);
	
	NewType( W3D_CHUNK_AABTREE, "W3D_CHUNK_AABTREE", List_W3D_CHUNK_AABTREE,true);
	NewType( W3D_CHUNK_AABTREE_HEADER, "W3D_CHUNK_AABTREE_HEADER", List_W3D_CHUNK_AABTREE_HEADER);
	NewType( W3D_CHUNK_AABTREE_POLYINDICES, "W3D_CHUNK_AABTREE_POLYINDICES", List_W3D_CHUNK_AABTREE_POLYINDICES);
	NewType( W3D_CHUNK_AABTREE_NODES, "W3D_CHUNK_AABTREE_NODES", List_W3D_CHUNK_AABTREE_NODES);
	
	NewType( W3D_CHUNK_HIERARCHY, "W3D_CHUNK_HIERARCHY", List_W3D_CHUNK_HIERARCHY, true);
	NewType( W3D_CHUNK_HIERARCHY_HEADER, "W3D_CHUNK_HIERARCHY_HEADER", List_W3D_CHUNK_HIERARCHY_HEADER);
	NewType( W3D_CHUNK_PIVOTS, "W3D_CHUNK_PIVOTS", List_W3D_CHUNK_PIVOTS);
	NewType( W3D_CHUNK_PIVOT_FIXUPS, "W3D_CHUNK_PIVOT_FIXUPS", List_W3D_CHUNK_PIVOT_FIXUPS);

	NewType( W3D_CHUNK_ANIMATION, "W3D_CHUNK_ANIMATION", List_W3D_CHUNK_ANIMATION, true);
	NewType( W3D_CHUNK_ANIMATION_HEADER, "W3D_CHUNK_ANIMATION_HEADER", List_W3D_CHUNK_ANIMATION_HEADER);
	NewType( W3D_CHUNK_ANIMATION_CHANNEL, "W3D_CHUNK_ANIMATION_CHANNEL", List_W3D_CHUNK_ANIMATION_CHANNEL);
	NewType( W3D_CHUNK_BIT_CHANNEL, "W3D_CHUNK_BIT_CHANNEL", List_W3D_CHUNK_BIT_CHANNEL);

	NewType( W3D_CHUNK_HMODEL, "W3D_CHUNK_HMODEL", List_W3D_CHUNK_HMODEL,true);
	NewType( W3D_CHUNK_HMODEL_HEADER, "W3D_CHUNK_HMODEL_HEADER", List_W3D_CHUNK_HMODEL_HEADER);
	NewType( W3D_CHUNK_NODE, "W3D_CHUNK_NODE", List_W3D_CHUNK_NODE);
	NewType( W3D_CHUNK_COLLISION_NODE, "W3D_CHUNK_COLLISION_NODE", List_W3D_CHUNK_COLLISION_NODE);
	NewType( W3D_CHUNK_SKIN_NODE, "W3D_CHUNK_SKIN_NODE", List_W3D_CHUNK_SKIN_NODE);
	NewType( OBSOLETE_W3D_CHUNK_HMODEL_AUX_DATA, "W3D_CHUNK_HMODEL_AUX_DATA", List_W3D_CHUNK_HMODEL_AUX_DATA);
	NewType( OBSOLETE_W3D_CHUNK_SHADOW_NODE, "OBSOLETE_W3D_CHUNK_SHADOW_NODE", List_W3D_CHUNK_SHADOW_NODE);

	NewType( W3D_CHUNK_LODMODEL, "W3D_CHUNK_LODMODEL", List_W3D_CHUNK_LODMODEL,true);
	NewType( W3D_CHUNK_LODMODEL_HEADER, "W3D_CHUNK_LODMODEL_HEADER", List_W3D_CHUNK_LODMODEL_HEADER);
	NewType( W3D_CHUNK_LOD, "W3D_CHUNK_LOD", List_W3D_CHUNK_LOD);

	NewType( W3D_CHUNK_COLLECTION, "W3D_CHUNK_COLLECTION", List_W3D_CHUNK_COLLECTION,true);
	NewType( W3D_CHUNK_COLLECTION_HEADER, "W3D_CHUNK_COLLECTION_HEADER", List_W3D_CHUNK_COLLECTION_HEADER);
	NewType( W3D_CHUNK_COLLECTION_OBJ_NAME, "W3D_CHUNK_COLLECTION_OBJ_NAME", List_W3D_CHUNK_COLLECTION_OBJ_NAME);
	NewType( W3D_CHUNK_PLACEHOLDER,"W3D_CHUNK_PLACEHOLDER", List_W3D_CHUNK_PLACEHOLDER);
	NewType( W3D_CHUNK_TRANSFORM_NODE,"W3D_CHUNK_TRANSFORM_NODE", List_W3D_CHUNK_TRANSFORM_NODE);

	NewType( W3D_CHUNK_POINTS, "W3D_CHUNK_POINTS", List_W3D_CHUNK_POINTS);

	NewType( W3D_CHUNK_LIGHT, "W3D_CHUNK_LIGHT", List_W3D_CHUNK_LIGHT,true);
	NewType( W3D_CHUNK_LIGHT_INFO, "W3D_CHUNK_LIGHT_INFO",List_W3D_CHUNK_LIGHT_INFO);
	NewType( W3D_CHUNK_SPOT_LIGHT_INFO, "W3D_CHUNK_SPOT_LIGHT_INFO",List_W3D_CHUNK_SPOT_LIGHT_INFO);
	NewType( W3D_CHUNK_NEAR_ATTENUATION, "W3D_CHUNK_NEAR_ATTENUATION", List_W3D_CHUNK_NEAR_ATTENUATION);
	NewType( W3D_CHUNK_FAR_ATTENUATION, "W3D_CHUNK_FAR_ATTENUATION", List_W3D_CHUNK_FAR_ATTENUATION);

	NewType( W3D_CHUNK_EMITTER, "W3D_CHUNK_EMITTER", List_W3D_CHUNK_EMITTER,true);
	NewType( W3D_CHUNK_EMITTER_HEADER, "W3D_CHUNK_EMITTER_HEADER",List_W3D_CHUNK_EMITTER_HEADER);
	NewType( W3D_CHUNK_EMITTER_USER_DATA, "W3D_CHUNK_EMITTER_USER_DATA", List_W3D_CHUNK_EMITTER_USER_DATA);
	NewType( W3D_CHUNK_EMITTER_INFO, "W3D_CHUNK_EMITTER_INFO", List_W3D_CHUNK_EMITTER_INFO);
	NewType( W3D_CHUNK_EMITTER_INFOV2, "W3D_CHUNK_EMITTER_INFOV2",List_W3D_CHUNK_EMITTER_INFOV2);
	NewType( W3D_CHUNK_EMITTER_PROPS, "W3D_CHUNK_EMITTER_PROPS", List_W3D_CHUNK_EMITTER_PROPS);
	NewType( OBSOLETE_W3D_CHUNK_EMITTER_COLOR_KEYFRAME, "OBSOLETE_W3D_CHUNK_EMITTER_COLOR_KEYFRAME", List_W3D_CHUNK_EMITTER_COLOR_KEYFRAME);
	NewType( OBSOLETE_W3D_CHUNK_EMITTER_OPACITY_KEYFRAME, "OBSOLETE_W3D_CHUNK_EMITTER_OPACITY_KEYFRAME", List_W3D_CHUNK_EMITTER_OPACITY_KEYFRAME);
	NewType( OBSOLETE_W3D_CHUNK_EMITTER_SIZE_KEYFRAME, "OBSOLETE_W3D_CHUNK_EMITTER_SIZE_KEYFRAME", List_W3D_CHUNK_EMITTER_SIZE_KEYFRAME);
	NewType( W3D_CHUNK_EMITTER_ROTATION_KEYFRAMES, "W3D_CHUNK_EMITTER_ROTATION_KEYFRAMES", List_W3D_CHUNK_EMITTER_ROTATION_KEYFRAMES);
	NewType( W3D_CHUNK_EMITTER_FRAME_KEYFRAMES, "W3D_CHUNK_EMITTER_FRAME_KEYFRAMES", List_W3D_CHUNK_EMITTER_FRAME_KEYFRAMES);
	NewType( W3D_CHUNK_EMITTER_BLUR_TIME_KEYFRAMES, "W3D_CHUNK_EMITTER_BLUR_TIME_KEYFRAMES", List_W3D_CHUNK_EMITTER_BLUR_TIME_KEYFRAMES);

	NewType( W3D_CHUNK_AGGREGATE, "W3D_CHUNK_AGGREGATE", List_W3D_CHUNK_AGGREGATE,true);
	NewType( W3D_CHUNK_AGGREGATE_HEADER, "W3D_CHUNK_AGGREGATE_HEADER", List_W3D_CHUNK_AGGREGATE_HEADER);
	NewType( W3D_CHUNK_AGGREGATE_INFO, "W3D_CHUNK_AGGREGATE_INFO",List_W3D_CHUNK_AGGREGATE_INFO);
	NewType( W3D_CHUNK_TEXTURE_REPLACER_INFO, "W3D_CHUNK_TEXTURE_REPLACER_INFO", List_W3D_CHUNK_TEXTURE_REPLACER_INFO);
	NewType( W3D_CHUNK_AGGREGATE_CLASS_INFO, "W3D_CHUNK_AGGREGATE_CLASS_INFO", List_W3D_CHUNK_AGGREGATE_CLASS_INFO);
	
	NewType( W3D_CHUNK_HLOD, "W3D_CHUNK_HLOD", List_W3D_CHUNK_HLOD, true);
	NewType( W3D_CHUNK_HLOD_HEADER, "W3D_CHUNK_HLOD_HEADER", List_W3D_CHUNK_HLOD_HEADER);
	NewType( W3D_CHUNK_HLOD_LOD_ARRAY, "W3D_CHUNK_HLOD_LOD_ARRAY", List_W3D_CHUNK_HLOD_LOD_ARRAY, true);
	NewType( W3D_CHUNK_HLOD_SUB_OBJECT_ARRAY_HEADER, "W3D_CHUNK_HLOD_LOD_ARRAY_HEADER", List_W3D_CHUNK_HLOD_LOD_ARRAY_HEADER);
	NewType( W3D_CHUNK_HLOD_SUB_OBJECT, "W3D_CHUNK_HLOD_SUB_OBJECT", List_W3D_CHUNK_HLOD_SUB_OBJECT);
	NewType( W3D_CHUNK_HLOD_AGGREGATE_ARRAY, "W3D_CHUNK_HLOD_AGGREGATE_ARRAY", List_W3D_CHUNK_HLOD_LOD_ARRAY, true);
	NewType( W3D_CHUNK_HLOD_PROXY_ARRAY, "W3D_CHUNK_HLOD_PROXY_ARRAY", List_W3D_CHUNK_HLOD_LOD_ARRAY, true);

	NewType( W3D_CHUNK_BOX, "W3D_CHUNK_BOX", List_W3D_CHUNK_BOX);

	NewType( W3D_CHUNK_NULL_OBJECT, "W3D_CHUNK_NULL_OBJECT", List_W3D_CHUNK_NULL_OBJECT);

	NewType( W3D_CHUNK_PRELIT_UNLIT,	"W3D_CHUNK_PRELIT_UNLIT", List_W3D_CHUNK_PRELIT_UNLIT, true);
	NewType( W3D_CHUNK_PRELIT_VERTEX, "W3D_CHUNK_PRELIT_VERTEX", List_W3D_CHUNK_PRELIT_VERTEX, true);
	NewType( W3D_CHUNK_PRELIT_LIGHTMAP_MULTI_PASS, "W3D_CHUNK_PRELIT_LIGHTMAP_MULTI_PASS", List_W3D_CHUNK_PRELIT_LIGHTMAP_MULTI_PASS, true);
	NewType( W3D_CHUNK_PRELIT_LIGHTMAP_MULTI_TEXTURE, "W3D_CHUNK_PRELIT_LIGHTMAP_MULTI_TEXTURE", List_W3D_CHUNK_PRELIT_LIGHTMAP_MULTI_TEXTURE, true);
	NewType( W3D_CHUNK_LIGHTSCAPE, "W3D_CHUNK_LIGHTSCAPE", List_W3D_CHUNK_LIGHTSCAPE, true);
	NewType( W3D_CHUNK_LIGHTSCAPE_LIGHT, "W3D_CHUNK_LIGHTSCAPE_LIGHT", List_W3D_CHUNK_LIGHTSCAPE_LIGHT, true);
	NewType( W3D_CHUNK_LIGHT_TRANSFORM, "W3D_CHUNK_LIGHT_TRANSFORM", List_W3D_CHUNK_LIGHT_TRANSFORM);

	NewType( W3D_CHUNK_DAZZLE, "W3D_CHUNK_DAZZLE", List_W3D_CHUNK_DAZZLE, true);
	NewType( W3D_CHUNK_DAZZLE_NAME, "W3D_CHUNK_DAZZLE_NAME" , List_W3D_CHUNK_DAZZLE_NAME);
	NewType( W3D_CHUNK_DAZZLE_TYPENAME, "W3D_CHUNK_DAZZLE_TYPENAME" , List_W3D_CHUNK_DAZZLE_TYPENAME);
}

ChunkType *ChunkTableClass::Lookup(int ID) {
	auto it = Types.find(ID);
	if(it != Types.end()) {
		return it->second;
	}
	return NULL;
}

ChunkItem::ChunkItem(ChunkLoadClass &cload) {
	ID = cload.Cur_Chunk_ID();
	Length = cload.Cur_Chunk_Length();
	Type = ChunkTable.Lookup(ID);

	// if the chunktype indicates that it has member chunks then do not load this chunk's data. Have the external caller do that.
	if(Type != 0 && Type->Wrapper) {
		Data = 0;
		return;
	}
	if(Type != 0) {
		printf("%s %d\n", Type->Name, Length);
	}

	if(Length == 0) {
		Data = 0;
	} else {
		Data = new uint8_t[Length];
		cload.Read(Data, Length);
	}
}

ChunkItem::~ChunkItem() {
	if(Data != 0) 
		delete [] Data;
	while(!Chunks.empty()) {
		ChunkItem *item = Chunks.front();
		Chunks.pop_front();
		delete item;
	}
}

ChunkData::ChunkData()
{

}

ChunkData::~ChunkData()
{
	Release_Data();
}

void ChunkData::Release_Data()
{
	while(!Chunks.empty()) {
		ChunkItem *item = Chunks.front();
		Chunks.pop_front();
		delete item;
	}
}

bool ChunkData::Load(const char *filename)
{
	Release_Data();

	RawFileClass chunk_file;

	if (!chunk_file.Open(filename)) {
		return false;
	}

	ChunkLoadClass cload(&chunk_file);

	while (cload.Open_Chunk()) {
		Add_Chunk(cload);
		cload.Close_Chunk();
	}

	chunk_file.Close();

	return true;
}


void ChunkData::Add_Chunk(ChunkLoadClass & cload, ChunkItem *Parent)
{
	ChunkItem *item = new ChunkItem(cload);
	item->Parent = Parent;
	if(Parent) 
		Parent->Chunks.push_back(item);
	else 
		Chunks.push_back(item);
//Moumine 1/2/2002    1:21:26 PM
#if 0//! defined _W3DSHELLEXT
	if(theApp.DumpTextures) {
		if(item->ID == W3D_CHUNK_TEXTURE_NAME) {
			static CMapStringToString existing;

			char * data = (char *)item->Data;
			
			CString value;
			if(existing.Lookup(data, value) == FALSE) {

				existing.SetAt(data, data);

				if(theApp.TextureDumpFile != 0) 
					fprintf(theApp.TextureDumpFile, "%s,%s\n", theApp.Filename, data);
				TRACE("%s,%s\n", theApp.Filename, data);
			}
		}
	}
#endif
	if((item->Type != 0) && item->Type->Wrapper) {
		while(cload.Open_Chunk()) {
			Add_Chunk(cload, item);
			cload.Close_Chunk();
		}
	}
}

int Get_Bit(void const * array, int bit)
{
	unsigned char mask = (unsigned char)(1 << (bit % 8));
	return((*((unsigned char *)array + (bit/8)) & mask) != 0);
}

void Set_Bit(void * array, int bit, int value)
{
	unsigned char mask = (unsigned char)(1 << (bit % 8));

	if (value != 0) {
		*((unsigned char *)array + (bit/8)) |= mask;
	} else {
		*((unsigned char *)array + (bit/8)) &= (unsigned char)~mask;
	}
}

ChunkDataModel::ChunkDataModel(ChunkData *chunkData, QObject *parent)
    : QAbstractItemModel(parent), m_chunkData(chunkData)
{
}

ChunkDataModel::~ChunkDataModel()
{
}

QModelIndex ChunkDataModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    const std::list<ChunkItem *> &children = parent.isValid()
        ? static_cast<ChunkItem *>(parent.internalPointer())->Chunks
        : m_chunkData->Chunks;

    auto it = children.begin();
    std::advance(it, row);
    return createIndex(row, column, *it);
}

QModelIndex ChunkDataModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    ChunkItem *item = static_cast<ChunkItem *>(index.internalPointer());
    ChunkItem *parentItem = item->Parent;
    if (parentItem == nullptr)
        return QModelIndex();

    const std::list<ChunkItem *> &grandparentChunks = parentItem->Parent
        ? parentItem->Parent->Chunks
        : m_chunkData->Chunks;

    int row = 0;
    for (auto it = grandparentChunks.begin(); it != grandparentChunks.end(); ++it, ++row) {
        if (*it == parentItem)
            return createIndex(row, 0, parentItem);
    }
    return QModelIndex();
}

int ChunkDataModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;
    if (!parent.isValid())
        return static_cast<int>(m_chunkData->Chunks.size());
    return static_cast<int>(static_cast<ChunkItem *>(parent.internalPointer())->Chunks.size());
}

int ChunkDataModel::columnCount(const QModelIndex &parent) const
{
    return 1; // Chunk Name
}

QVariant ChunkDataModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    ChunkItem *item = static_cast<ChunkItem *>(index.internalPointer());
	return item->Type ? QString(item->Type->Name) : QString("Unknown");
}
