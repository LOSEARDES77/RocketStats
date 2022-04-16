#include "../RocketStats.h"

void RocketStats::GameStart(std::string eventName)
{
    if (!is_online_game || is_game_started)
        return;

    cvarManager->log("===== GameStart =====");

    CarWrapper me = gameWrapper->GetLocalCar();
    if (me.IsNull())
        return;

    PriWrapper mePRI = me.GetPRI();
    if (mePRI.IsNull())
        return;

    TeamInfoWrapper myTeam = mePRI.GetTeam();
    if (myTeam.IsNull())
        return;

    MMRWrapper mmrw = gameWrapper->GetMMRWrapper();
    current.playlist = mmrw.GetCurrentPlaylist();
    SkillRank playerRank = mmrw.GetPlayerRank(gameWrapper->GetUniqueID(), current.playlist);
    cvarManager->log(std::to_string(current.playlist) + " -> " + GetPlaylistName(current.playlist));

    // Get TeamNum
    my_team_num = myTeam.GetTeamNum();

    // Set Game Started
    is_game_started = true;
    is_game_ended = false;

    UpdateMMR(gameWrapper->GetUniqueID());
    WriteConfig();
    UpdateFiles();

    cvarManager->log("===== !GameStart =====");
}

void RocketStats::GameEnd(std::string eventName)
{
    cvarManager->log("GameEnd => is_online_game:" + std::to_string(is_online_game) + " my_team_num:" + std::to_string(my_team_num));
    if (is_online_game && my_team_num != -1)
    {
        cvarManager->log("===== GameEnd =====");
        ServerWrapper server = gameWrapper->GetOnlineGame();
        TeamWrapper winningTeam = server.GetGameWinner();
        if (winningTeam.IsNull())
            return;

        int teamnum = winningTeam.GetTeamNum();

        // Game as ended
        is_game_ended = true;

        ++always.games;
        ++session.games;
        ++stats[current.playlist].games;
        ++always_gm[current.playlist].games;

        cvarManager->log("GameEnd => my_team_num:" + std::to_string(my_team_num) + " GetTeamNum:" + std::to_string(winningTeam.GetTeamNum()));
        if (my_team_num == winningTeam.GetTeamNum())
        {
            cvarManager->log("===== Game Won =====");
            // On Win, Increase streak and Win Number
            ++always.win;
            ++session.win;
            ++stats[current.playlist].win;
            ++always_gm[current.playlist].win;

            cvarManager->log("GameEnd => streak:" + std::to_string(stats[current.playlist].streak));
            if (stats[current.playlist].streak < 0)
            {
                always.streak = 0;
                session.streak = 0;
                stats[current.playlist].streak = 0;
                always_gm[current.playlist].streak = 0;
            }

            ++always.streak;
            ++session.streak;
            ++stats[current.playlist].streak;
            ++always_gm[current.playlist].streak;

            SetRefresh(RefreshFlags_Refresh);
            WriteWin();
        }
        else
        {
            cvarManager->log("===== Game Lost =====");
            // On Loose, Increase loose Number and decrease streak
            ++always.loss;
            ++session.loss;
            ++stats[current.playlist].loss;
            ++always_gm[current.playlist].loss;

            cvarManager->log("GameEnd => streak:" + std::to_string(stats[current.playlist].streak));
            if (stats[current.playlist].streak > 0)
            {
                always.streak = 0;
                session.streak = 0;
                stats[current.playlist].streak = 0;
                always_gm[current.playlist].streak = 0;
            }

            --always.streak;
            --session.streak;
            --stats[current.playlist].streak;
            --always_gm[current.playlist].streak;

            SetRefresh(RefreshFlags_Refresh);
            WriteLoss();
        }

        WriteGames();
        WriteStreak();
        WriteBoost();
        ResetBasicStats();
        WriteConfig();

        // Reset myTeamNum security
        my_team_num = -1;

        gameWrapper->SetTimeout([&](GameWrapper* gameWrapper) { UpdateMMR(gameWrapper->GetUniqueID()); }, 3.0F);

        cvarManager->log("===== !GameEnd =====");
    }
}

void RocketStats::GameDestroyed(std::string eventName)
{
    cvarManager->log("===== GameDestroyed =====");

    // Check if Game Ended, if not, RAGE QUIT or disconnect
    cvarManager->log("GameDestroyed => is_game_started:" + std::to_string(is_game_started) + " is_game_ended:" + std::to_string(is_game_ended));
    if (is_game_started == true && is_game_ended == false)
    {
        ++always.games;
        ++session.games;
        ++stats[current.playlist].games;
        ++always_gm[current.playlist].games;

        ++always.loss;
        ++session.loss;
        ++stats[current.playlist].loss;
        ++always_gm[current.playlist].loss;

        cvarManager->log("GameDestroyed => streak:" + std::to_string(stats[current.playlist].streak));
        if (stats[current.playlist].streak > 0)
        {
            always.streak = 0;
            session.streak = 0;
            stats[current.playlist].streak = 0;
            always_gm[current.playlist].streak = 0;
        }

        --always.streak;
        --session.streak;
        --stats[current.playlist].streak;
        --always_gm[current.playlist].streak;

        WriteGames();
        WriteStreak();
        WriteLoss();
        ResetBasicStats();
        WriteConfig();
    }

    is_game_ended = true;
    is_game_started = false;
    WriteBoost();

    SetRefresh(RefreshFlags_Refresh);
    cvarManager->log("===== !GameDestroyed =====");
}

int RocketStats::GetGameTime()
{
    ServerWrapper server = gameWrapper->GetGameEventAsServer();

    if (!is_in_game)
    {
        server = gameWrapper->GetOnlineGame();

        if (is_offline_game)
            return -1;
        else if (server.IsNull())
            return -1;

        return server.GetSecondsRemaining();
    }
    else if (server.IsNull())
        return -1;

    return server.GetSecondsRemaining(); // max game time: GetGameTime, seconds elapsed: GetSecondsElapsed
}

TeamWrapper RocketStats::GetTeam(bool opposing)
{
    if (is_in_game)
    {
        ServerWrapper server = (is_online_game ? gameWrapper->GetOnlineGame() : gameWrapper->GetGameEventAsServer());

        if (!server.IsNull())
        {
            ArrayWrapper<TeamWrapper> teams = server.GetTeams();
            PlayerControllerWrapper player = server.GetLocalPrimaryPlayer();

            if (!teams.IsNull() && !player.IsNull())
            {
#pragma warning(push)
#pragma warning(disable: 4267)
                for (TeamWrapper team : teams)
#pragma warning(pop)
                {
                    if (opposing == (player.GetTeamNum2() != team.GetTeamNum2()))
                        return team;
                }
            }
        }
    }
}

ImColor RocketStats::GetTeamColor(TeamWrapper team)
{
    if (!team.IsNull())
    {
        LinearColor color = team.GetCurrentColorList().Get(0);
        return ImColor{ (color.R / 255.f), (color.G / 255.f), (color.B / 255.f) };
    }
}