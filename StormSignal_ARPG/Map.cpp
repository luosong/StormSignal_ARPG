﻿#include <string>
#include <cwctype>
#include <DxLib.h>
#include <Box2D\Box2D.h>
#include "Map.h"
#include "Functions.h"
#include "ConstantValue.h"
#include "HitBox.h"
#include "SE.h"

void Map::Initialize(b2World *World,bool InitPlayerFlag)
{
	PauseFlag = false;
	MessageFlag = false;
	NextStageName = "";
	for(int i=0;i<14;i++)MapData[i].clear();
	ScriptData.clear();
	MapChipFixtures.clear();
	FixtureDataToMapChip.clear();
	MapChips.clear();
	EnemyData.clear();
	RigidBodies.clear();
	MessageWindow.Initialize(Screen_Width/2-150,Screen_Height/2-100,300,200,Black,LightBlack);
	PlayerData.x = PlayerData.y = 0;

	//フォント読み込み
	DeleteFontToHandle(FontSmall);
	FontSmall = CreateFontToHandle( "メイリオ" , 15 , 3 ,-1,-1,2) ;
	DeleteFontToHandle(FontMiddle);
	FontMiddle = CreateFontToHandle( "メイリオ" , 20 , 7 ,-1,-1,2) ;
	DeleteFontToHandle(FontBig);
	FontBig = CreateFontToHandle( "メイリオ" , 40 , 10 ,-1,-1,3) ;

	Image_2D TempGraph;

	//空欄 0
	MapChips.push_back(TempGraph);

	//土 1
	MapChips.push_back(TempGraph);
	MapChips[MapChips.size()-1].Load("Image/Map/Clay.png");

	//土上 2
	MapChips.push_back(TempGraph);
	MapChips[MapChips.size()-1].Load("Image/Map/ClayFloor.png");

	//木箱 3
	MapChips.push_back(TempGraph);
	MapChips[MapChips.size()-1].Load("Image/Map/Woodbox.png");

	//スイッチ1(off) 4
	MapChips.push_back(TempGraph);
	MapChips[MapChips.size()-1].Load("Image/Map/Switch1.png");

	//スイッチ2(on) 5
	MapChips.push_back(TempGraph);
	MapChips[MapChips.size()-1].Load("Image/Map/Switch2.png");

	//看板 6
	MapChips.push_back(TempGraph);
	MapChips[MapChips.size()-1].Load("Image/Map/Board.png");

	//旗 7
	MapChips.push_back(TempGraph);
	MapChips[MapChips.size()-1].Load("Image/Map/Flag.png");

	//点線ブロック 8
	MapChips.push_back(TempGraph);
	MapChips[MapChips.size()-1].Load("Image/Map/BlankBlock.png");

	//Interface
	Interfaces.push_back(TempGraph);
	Interfaces[Interfaces.size()-1].Load("Image/Interface/HPFrame.png");
	Interfaces[Interfaces.size()-1].Initialize();

	Interfaces.push_back(TempGraph);
	Interfaces[Interfaces.size()-1].Load("Image/Interface/HPBar.png");
	Interfaces[Interfaces.size()-1].Initialize();

	Interfaces.push_back(TempGraph);
	Interfaces[Interfaces.size()-1].Load("Image/Interface/EXFrame.png");
	Interfaces[Interfaces.size()-1].Initialize();

	Interfaces.push_back(TempGraph);
	Interfaces[Interfaces.size()-1].Load("Image/Interface/EXBar.png");
	Interfaces[Interfaces.size()-1].Initialize();

	if(InitPlayerFlag)
	{
		PlayerData.Load("Image/Chara/None.png");
		PlayerData.Initialize(World,Mapchip_Player,1,0,100);
	}

}

void Map::LoadMapData(string Pass)
{
	StagePass = Pass;
	int LineData = FileRead_open(Pass.c_str());
	//マップの横幅を計算する
	Width = FileRead_size(Pass.c_str())/14+2;
	for(int i=0;i<14;i++)
	{
		char *Data;
		Data = (char*)malloc(Width);
		FileRead_gets(Data,Width,LineData);
		MapData[i] = split(Data,",");
		free(Data);

	}
	FileRead_close(LineData);
	Width = MapData[0].size();
}

void Map::LoadScriptData(string Pass)
{
	ScriptPass = Pass;
	int LineData = FileRead_open(Pass.c_str());
	while(FileRead_eof(LineData) == 0)
	{
		char Data[256];
		FileRead_gets(Data,256,LineData);
		ScriptData.push_back(split(Data,","));
	}
	FileRead_close(LineData);
}

void Map::CreateMap(b2World *World)
{
	//当たり判定生成準備
	GroundBodyDef.position.Set(0,0);
	GroundBody = World->CreateBody(&GroundBodyDef);
	GroundBody->SetUserData("Ground");

	vector<b2Vec2> VectList;

	for(int y=0;y<14;y++)
	{
		for(int x=0;x<Width;x++)
		{
			//改行コードの削除(2文字より大きい場合は切り捨てる)
			if(MapData[y][x].size() > 2)MapData[y][x].erase(MapData[y][x].begin()+2,MapData[y][x].end());

			//マップデータとマップチップを照らし合わせ、一致したものを設置していく
			if(MapData[y][x] == Mapchip_Blank)continue;
			else if(MapData[y][x] == Mapchip_Player)
			{
				PlayerData.GetBody()->SetTransform(b2Vec2((x*32+16)/Box_Rate,(y*32+16)/Box_Rate),0);
				continue;
			}else if(MapData[y][x] == Mapchip_Woodbox)
			{
				Object Body;
				RigidBodies.push_back(Body);
				RigidBodies[RigidBodies.size()-1].Graph = MapChips[3].Graph;
				RigidBodies[RigidBodies.size()-1].SIHandle = MapChips[3].SIHandle;
				RigidBodies[RigidBodies.size()-1].Initialize(World,"Mapchip_Woodbox",1,1,-1);
				RigidBodies[RigidBodies.size()-1].GetBody()->SetTransform(b2Vec2((x*32+16)/Box_Rate,(y*32+16)/Box_Rate),0);
				continue;
			}else if(MapData[y][x] == Mapchip_TrainingBag
				||	MapData[y][x] == Mapchip_CommonEnemy1)
			{
				Enemy EnemyTemp;
				EnemyData.push_back(EnemyTemp);
				int HP = 0;
				if(MapData[y][x] == Mapchip_TrainingBag)HP = INT_MAX;
				else if(MapData[y][x] == Mapchip_CommonEnemy1)HP = 100;
				EnemyData[EnemyData.size()-1].Initialize(World,MapData[y][x],1,1,HP);
				EnemyData[EnemyData.size()-1].GetBody()->SetTransform(b2Vec2((x*32+16)/Box_Rate,(y*32+16)/Box_Rate),0);
				continue;
			}

			//特殊ブロック
			if(isalpha(static_cast<unsigned char>(MapData[y][x][0])))
			{
				int Length = ScriptData.size();
				int Flag = -1;
				for(int i=0;i<Length;i++)
				{
					if(ScriptData[i][0] == MapData[y][x])
					{
						if(ScriptData[i][2] == "1")
						{
							Flag = i;
							break;
						}
					}
				}
				if(Flag >= 0)
				{
					//当たり判定の生成
					GroundBox.SetAsBox(16/Box_Rate-0.1,16/Box_Rate-0.1,b2Vec2((x*32+16)/Box_Rate,(y*32+16)/Box_Rate),0);
					MapChipFixtures.push_back(GroundBody->CreateFixture(&GroundBox,0.f));
					string Str = ScriptData[Flag][0] + "," + ntos(MapChipFixtures.size()-1);
					FixtureDataToMapChip.push_back(Str);
				}
				continue;
			}

			//当たり判定の生成
			GroundBox.SetAsBox(16.f/Box_Rate-0.1,16.f/Box_Rate-0.1,b2Vec2((x*32.f+16.f)/Box_Rate,(y*32.f+16.f)/Box_Rate),0);
			GroundBody->CreateFixture(&GroundBox,0.f);
		}
	}
	//GroundBody->CreateFixture(&GroundBox,0.f);
}

void Map::DestroyAll(b2World *World)
{
	World->DestroyBody(GroundBody);
	World->DestroyBody(PlayerData.GetBody());
	PlayerData.GetBody()->SetLinearVelocity(b2Vec2(0,0));
	PlayerData.DeleteCharacterList();
	PlayerData.Unload();
	int Length = EnemyData.size();
	for(int i=0;i<Length;i++)
	{
		EnemyData[i].DestroyBody();
	}
	EnemyData.clear();
	Length = RigidBodies.size();
	for(int i=0;i<Length;i++)
	{
		RigidBodies[i].DestroyBody();
	}
	RigidBodies.clear();
}

void Map::DestroyMap(b2World *World)
{
	World->DestroyBody(GroundBody);
	PlayerData.GetBody()->SetLinearVelocity(b2Vec2(0,0));
	int Length = EnemyData.size();
	for(int i=0;i<Length;i++)
	{
		EnemyData[i].DestroyBody();
	}
	EnemyData.clear();
	Length = RigidBodies.size();
	for(int i=0;i<Length;i++)
	{
		RigidBodies[i].DestroyBody();
	}
	RigidBodies.clear();
}

bool Map::GetPauseFlag()
{
	return PauseFlag;
}

bool Map::GetMessageFlag()
{
	return MessageFlag;
}

string Map::GetNextStageName()
{
	return NextStageName;
	
}

int Map::GetPlayerHP()
{
	return PlayerData.HP;
}

void Map::InitPlayerHP()
{
	PlayerData.HP = PlayerData.MaxHP;
}

string Map::GetStagePass()
{
	return StagePass;
}

string Map::GetScriptPass()
{
	return ScriptPass;
}

void Map::Step()
{
	//ポーズ処理,メッセージ表示
	if(MessageFlag)
	{
		if(!PauseFlag)
		{
			MessageWindow.Draw();
			MessageWindow.Ext -= 0.02f;
			if(MessageWindow.Ext < 0)
			{
				MessageFlag = false;
			}
			return;
		}else
		{
			MessageWindow.Draw();
			if(MessageWindow.Ext < 1.f)MessageWindow.Ext += 0.02f;
			else if(CheckKeyDown(KEY_INPUT_UP))
			{
				PauseFlag = false;
				return;
			}
		}
	}else
	{
		PauseFlag = PlayerData.GetSkillWindowVisible();
	}
	if(PauseFlag)return;

	//プレイヤーの処理
	PlayerData.Ctrl();
	PlayerData.Step();

	//敵の処理
	int Length = EnemyData.size();
	for(int i=0;i<Length;i++)
	{
		EnemyData[i].Ctrl();
		EnemyData[i].Step();

		if(EnemyData[i].HP < 0)
		{
			EnemyData[i].DestroyBody();
			EnemyData.erase(EnemyData.begin()+i);
			i--;
			Length--;
		}
	}

	/*
		マップのスクロール
		左端、右端にプレイヤーが位置していた場合、マップの方を移動させる
	*/
	b2Transform PlayerTrans = PlayerData.GetBody()->GetTransform();
	b2Transform MapTrans = GroundBody->GetTransform();

	if(PlayerTrans.p.x < Screen_Width*(3.0/7.0)/Box_Rate)
	{
		float ScrollDistance = Screen_Width*(3.0/7.0)/Box_Rate - PlayerTrans.p.x;
		MapTrans.p.x += ScrollDistance;
		PlayerTrans.p.x += ScrollDistance;

		int Length = PlayerData.CharacterList.size();
		for(int i=0;i<Length;i++)
		{
			b2Transform Trans = PlayerData.CharacterList[i]->GetBody()->GetTransform();
			Trans.p.x += ScrollDistance;
			PlayerData.CharacterList[i]->GetBody()->SetTransform(Trans.p,Trans.q.GetAngle());
		}

		Length = PlayerData.HitBoxList.size();
		for(int i=0;i<Length;i++)
		{
			b2Transform Trans = PlayerData.HitBoxList[i].GetTransform();
			Trans.p.x += ScrollDistance;
			PlayerData.HitBoxList[i].SetTransform(Trans);
		}

		GroundBody->SetTransform(MapTrans.p,MapTrans.q.GetAngle());
	}else if(PlayerTrans.p.x > Screen_Width*(4.0/7.0)/Box_Rate)
	{
		float ScrollDistance = PlayerTrans.p.x - Screen_Width*(4.0/7.0)/Box_Rate;
		MapTrans.p.x -= ScrollDistance;

		int Length = PlayerData.CharacterList.size();
		for(int i=0;i<Length;i++)
		{
			b2Transform Trans = PlayerData.CharacterList[i]->GetBody()->GetTransform();
			Trans.p.x -= ScrollDistance;
			PlayerData.CharacterList[i]->GetBody()->SetTransform(Trans.p,Trans.q.GetAngle());
		}

		Length = PlayerData.HitBoxList.size();
		for(int i=0;i<Length;i++)
		{
			b2Transform Trans = PlayerData.HitBoxList[i].GetTransform();
			Trans.p.x -= ScrollDistance;
			PlayerData.HitBoxList[i].SetTransform(Trans);
		}

		GroundBody->SetTransform(MapTrans.p,MapTrans.q.GetAngle());
	}

	//特殊ブロックの処理
	for(int y=0;y<14;y++)
	{
		for(int x=0;x<Width;x++)
		{
			//特殊ブロック
			if(isalpha(static_cast<unsigned char>(MapData[y][x][0])))
			{
				int Length = ScriptData.size();
				for(int i=0;i<Length;i++)
				{
					if(ScriptData[i][0] == MapData[y][x])
					{
						int Flag = false;
						//トリガー
						if(ScriptData[i][3].find(Trigger_Hit) != string::npos)
						{//対象に触れた瞬間のみ起動
							if(PlayerData.HitTestRect(MapTrans.p.x*Box_Rate+x*32,y*32,32,32,true))
							{
								if(ScriptData[i][3] != Trigger_Hitted)
								{
									Flag = true;
									ScriptData[i][3] = Trigger_Hitted;
								}
							}else
							{
								ScriptData[i][3] = Trigger_Hit;
							}
						}else if(ScriptData[i][3] == Trigger_Touch)
						{//対象に触れている間常に起動
							if(PlayerData.HitTestRect(MapTrans.p.x*Box_Rate+x*32,y*32,32,32,true))
							{
								Flag = true;
							}
						}else if(ScriptData[i][3] == Trigger_Use)
						{//対象の近くで↑キーを押すと起動
							int Dis = GetDistance(PlayerData.x,PlayerData.y,(int)(MapTrans.p.x*Box_Rate+x*32+16),y*32+16);
							if(GetDistance(PlayerData.x,PlayerData.y,(int)(MapTrans.p.x*Box_Rate+x*32+16),y*32+16) < 50
								&&  CheckHitKey(KEY_INPUT_UP))
							{
								Flag = true;
							}
						}else if(ScriptData[i][3] == Trigger_Flagged)
						{
							Flag = true;
							ScriptData[i][3] == Trigger_Flag;
						}

						if(ScriptData[i].size() == 7)
						{//インターバル用領域追加
							ScriptData[i].push_back(ntos(atoi(ScriptData[i][6].c_str())+GetNowCount()));
						}
						int Count = atoi(ScriptData[i][5].c_str());
						if(Flag && (Count > 0 || Count <= -1) && atoi(ScriptData[i][7].c_str()) < GetNowCount())
						{
							Count--;
							ScriptData[i][5] = ntos(Count);
							ScriptData[i][7] = ntos(GetNowCount()+atoi(ScriptData[i][6].c_str()));
							if(ScriptData[i][4].find(Action_Flag) != string::npos)
							{//Targetのフラグを立てる
								vector<string> Data = split(ScriptData[i][4],"|");
								for(int j=0;j<Length;j++)
								{
									if(ScriptData[j][0] == Data[1] && ScriptData[j][3] == Trigger_Flag)
									{
										ScriptData[j][3] = Trigger_Flagged;
									}
								}
							}else if(ScriptData[i][4].find(Action_Redraw) != string::npos)
							{//自分の見た目をNumberへ変更する
								vector<string> Data = split(ScriptData[i][4],"|");
								ScriptData[i][1] = Data[1];
							}else if(ScriptData[i][4].find(Action_Replace) != string::npos)
							{//自分を指定した文字の特殊ブロックへ置き換える
								vector<string> Data = split(ScriptData[i][4],"|");
								MapData[y][x] = Data[1];
							}else if(ScriptData[i][4] == Action_Delete)
							{//自分をマップから削除する
								int FixLength = FixtureDataToMapChip.size();
								for(int j=0;j<FixLength;j++)
								{
									vector<string> Data = split(FixtureDataToMapChip[j],",");
									if(Data[0] == MapData[y][x])
									{//当たり判定を削除
										GroundBody->DestroyFixture(MapChipFixtures[atoi(Data[1].c_str())]);
									}
								}
								MapData[y][x] = Mapchip_Blank;
							}else if(ScriptData[i][4].find(Action_Clear) != string::npos)
							{//現在プレイしているステージを完了し、次のステージへ移動する
								vector<string> Data = split(ScriptData[i][4],"|");
								MessageFlag = true;
								NextStageName = Data[1];
							}else if(ScriptData[i][4].find(Action_Sound) != string::npos)
							{//指定サウンドを鳴らす
								vector<string> Data = split(ScriptData[i][4],"|");
								SetVolumeSound(SEManager.GetVolume()*2.55);
								PlaySound(Data[1].c_str(),DX_PLAYTYPE_BACK);
							}else if(ScriptData[i][4].find(Action_Message) != string::npos)
							{//ゲームの進行を止め、ShowMessageを表示する
								vector<string> Data = split(ScriptData[i][4],"|");
								MessageFlag = true;
								PauseFlag = true;

								MessageWindow.ReWindow();
								MessageWindow.DrawStringInWindow(5,5,DrawString_Left,Data[1],FontSmall,White);
								MessageWindow.Ext = 0.01f;
							}else if(ScriptData[i][4].find(Action_Collision) != string::npos)
							{//自分の当たり判定を変更する
								vector<string> Data = split(ScriptData[i][4],"|");
								if(Data[1] == "0")
								{//当たり判定を消す
									int FixLength = FixtureDataToMapChip.size();
									for(int j=0;j<FixLength;j++)
									{
										vector<string> Data = split(FixtureDataToMapChip[j],",");
										if(Data[0] == MapData[y][x])
										{//当たり判定を削除
											GroundBody->DestroyFixture(MapChipFixtures[atoi(Data[1].c_str())]);
										}
									}
								}else if(Data[1] == "1")
								{//当たり判定を追加する
									GroundBox.SetAsBox(16/Box_Rate-0.1,16/Box_Rate-0.1,b2Vec2((x*32+16)/Box_Rate,(y*32+16)/Box_Rate),0);
									MapChipFixtures.push_back(GroundBody->CreateFixture(&GroundBox,0.f));
									string Str = ScriptData[i][0] + "," + ntos(MapChipFixtures.size()-1);
									FixtureDataToMapChip.push_back(Str);
								}
							}else if(ScriptData[i][4].find(Action_Clear) != string::npos)
							{//現在プレイしているステージを完了し、次のステージへ移動する
								vector<string> Data = split(ScriptData[i][4],"|");
								MessageFlag = true;
								NextStageName = Data[1];
							}
						}
					}
				}
			}
		}
	}
}

void Map::Draw()
{
	//マップのスクロール分を取得
	b2Transform MapTrans = GroundBody->GetTransform();
	for(int y=0;y<14;y++)
	{
		for(int x=0;x<Width;x++)
		{
			string GraphNum = MapData[y][x];

			//特殊ブロック
			if(isalpha(static_cast<unsigned char>(MapData[y][x][0])))
			{
				int Length = ScriptData.size();
				for(int i=0;i<Length;i++)
				{
					if(ScriptData[i][0] == MapData[y][x])
					{
						GraphNum = ScriptData[i][1];
						break;
					}
				}
			}

			if(GraphNum == Mapchip_Blank)continue;
			int Graph = -1;
			if(GraphNum == Mapchip_Clay)Graph = MapChips[1].Graph[0];
			if(GraphNum == Mapchip_ClayFloor)Graph = MapChips[2].Graph[0];
			if(GraphNum == Mapchip_Woodbox)continue;
			if(GraphNum == Mapchip_Switch1)Graph = MapChips[4].Graph[0];
			if(GraphNum == Mapchip_Switch2)Graph = MapChips[5].Graph[0];
			if(GraphNum == Mapchip_Board)Graph = MapChips[6].Graph[0];
			if(GraphNum == Mapchip_Flag)Graph = MapChips[7].Graph[0];
			if(GraphNum == Mapchip_BlankBlock)Graph = MapChips[8].Graph[0];

			DrawGraph(MapTrans.p.x*Box_Rate+x*32,y*32,Graph,true);
		}
	}

	//プレイヤーの描画
	PlayerData.Draw(true);
	PlayerData.SetScrollDistance(MapTrans.p.x);

	//敵の描画
	int Length = EnemyData.size();
	SetUseMaskScreenFlag(true);
	for(int i=0;i<Length;i++)
	{
		EnemyData[i].Draw();
		EnemyData[i].SetScrollDistance(MapTrans.p.x);

		Interfaces[Interface_HPFrame].x = Interfaces[Interface_HPBar].x = EnemyData[i].x;
		Interfaces[Interface_HPFrame].y = Interfaces[Interface_HPBar].y = EnemyData[i].y+EnemyData[i].Height/2;
		Interfaces[Interface_HPFrame].Ext_x = Interfaces[Interface_HPBar].Ext_x = EnemyData[i].Width/79.0*2;
		Interfaces[Interface_HPFrame].Ext_y = Interfaces[Interface_HPBar].Ext_y = 0.6f;
		Interfaces[Interface_HPFrame].Draw(true);

		#pragma region マスクの作成
		int Width = Interfaces[Interface_HPFrame].Center_x*2*(EnemyData[i].Width/79.0*2);
		int Height = Interfaces[Interface_HPFrame].Center_y*2*0.6;
		int Mask = MakeMask(Width,Height);
		double Per = (double)EnemyData[i].HP / (double)EnemyData[i].MaxHP;

		//2次元配列を動的に、連続したメモリ領域に作成する
		unsigned char **Data = new unsigned char*[Height];
		Data[0] = new unsigned char[Width*Height];
		for(int j=1;j<Height;j++)
		{
			Data[j] = Data[0] + j * Width;
		}

		for(int j=0;j<Height;j++)
		{
			for(int k=0;k<Width;k++)
			{
				if(k<=Width*Per)Data[j][k] = 0x00;
				else Data[j][k] = 0xff;
			}
		}
		SetDataToMask(Width,Height,*Data,Mask);
		#pragma endregion
		DrawMask(EnemyData[i].x-Width/2,EnemyData[i].y+EnemyData[i].Height/2-Height/2,Mask,DX_MASKTRANS_BLACK);
		Interfaces[Interface_HPBar].Draw(true);
		DeleteMask(Mask);
		FillMaskScreen(0);

		delete[] Data[0];
		delete[] Data;
	}

	SetUseMaskScreenFlag(false);
	//オブジェクトの描画
	Length = RigidBodies.size();
	for(int i=0;i<Length;i++)
	{
		RigidBodies[i].Draw();
		RigidBodies[i].SetScrollDistance(MapTrans.p.x);
	}

	//ヒットボックスの描画
	Length = PlayerData.HitBoxList.size();
	for(int i=0;i<Length;i++)
	{
		PlayerData.HitBoxList[i].Draw();
	}

	//エフェクトの描画
	HitBox TempBox;
	Length = TempBox.Effects.size();
	for(int i=0;i<Length;i++)
	{
		if(TempBox.Effects[i].Draw(true))
		{
			TempBox.Effects.erase(TempBox.Effects.begin()+i);
			i--;
			Length--;
		}
	}

	Length = PlayerData.Effects.size();
	for(int i=0;i<Length;i++)
	{
		if(PlayerData.Effects[i].Draw(true))
		{
			PlayerData.Effects.erase(PlayerData.Effects.begin()+i);
			i--;
			Length--;
		}
	}

	//プレイヤーHPの描画
	Interfaces[Interface_HPFrame].x = Interfaces[Interface_HPBar].x = 165;
	Interfaces[Interface_HPFrame].y = Interfaces[Interface_HPBar].y = 20;
	Interfaces[Interface_HPFrame].Ext_x = Interfaces[Interface_HPBar].Ext_x = 4.f;
	Interfaces[Interface_HPFrame].Ext_y = Interfaces[Interface_HPBar].Ext_y = 1.5f;
	Interfaces[Interface_HPFrame].Draw(true);
	SetUseMaskScreenFlag(true);
	#pragma region マスクの作成
	int Width = Interfaces[Interface_HPFrame].Center_x*2*4;
	int Height = Interfaces[Interface_HPFrame].Center_y*2*1.5;
	int Mask = MakeMask(Width,Height);
	double Per = (double)PlayerData.HP / (double)PlayerData.MaxHP;

	//2次元配列を動的に、連続したメモリ領域に作成する
	unsigned char **Data = new unsigned char*[Height];
	Data[0] = new unsigned char[Width*Height];
	for(int j=1;j<Height;j++)
	{
		Data[j] = Data[0] + j * Width;
	}

	for(int i=0;i<Height;i++)
	{
		for(int j=0;j<Width;j++)
		{
			if(j<=Width*Per)Data[i][j] = 0x00;
			else Data[i][j] = 0xff;
		}
	}
	SetDataToMask(Width,Height,*Data,Mask);
	delete[] Data[0];
	delete[] Data;
	#pragma endregion
	DrawMask(5,5,Mask,DX_MASKTRANS_BLACK);
	Interfaces[Interface_HPBar].Draw(true);
	DeleteMask(Mask);
	FillMaskScreen(0);
	SetUseMaskScreenFlag(false);

	//プレイヤーEXゲージの描画
	Interfaces[Interface_EXFrame].Ext_x = Interfaces[Interface_EXBar].Ext_x = 1.f;
	Interfaces[Interface_EXFrame].Ext_y = Interfaces[Interface_EXBar].Ext_y = 1.f;
	int Count = (int)((double)PlayerData.GetEXGauge() / 100.0+0.99);
	for(int i=0;i<4;i++)
	{
		Interfaces[Interface_EXFrame].x = Interfaces[Interface_EXBar].x = 45 + i*80;
		Interfaces[Interface_EXFrame].y = Interfaces[Interface_EXBar].y = 50;
		Interfaces[Interface_EXFrame].Draw(true);
		if(i < Count)
		{
			SetUseMaskScreenFlag(true);
			#pragma region マスクの作成
			int Width = Interfaces[Interface_EXFrame].Center_x*2;
			int Height = Interfaces[Interface_EXFrame].Center_y*2;
			int Mask = MakeMask(Width,Height);
			double Per = PlayerData.GetEXGauge()%100 / 100.0;
			if(i < Count-1 || PlayerData.GetEXGauge() - (Count-1)*100 == 100)Per = 1;
			//2次元配列を動的に、連続したメモリ領域に作成する
			unsigned char **Data = new unsigned char*[Height];
			Data[0] = new unsigned char[Width*Height];
			for(int j=1;j<Height;j++)
			{
				Data[j] = Data[0] + j * Width;
			}

			for(int i=0;i<Height;i++)
			{
				for(int j=0;j<Width;j++)
				{
					if(j<=Width*Per)Data[i][j] = 0x00;
					else Data[i][j] = 0xff;
				}
			}
			SetDataToMask(Width,Height,*Data,Mask);
			delete[] Data[0];
			delete[] Data;
			#pragma endregion
			DrawMask(5 + i*80,43,Mask,DX_MASKTRANS_BLACK);
			Interfaces[Interface_EXBar].Draw(true);
			DeleteMask(Mask);
			FillMaskScreen(0);
			SetUseMaskScreenFlag(false);
		}
	}
	if(PlayerData.ComboCount > 0)
	{//コンボ数の描画
		string Str = ntos(PlayerData.ComboCount) + " Combo!";
		DrawStringToHandle(5,80,Str.c_str(),White,FontMiddle);
	}


	//スキル設定ウインドウの処理
	if(!MessageFlag)
	{
		PlayerData.StepSkillWindow();
	}

	//メッセージウィンドウ
	if(MessageFlag)MessageWindow.Draw();
}