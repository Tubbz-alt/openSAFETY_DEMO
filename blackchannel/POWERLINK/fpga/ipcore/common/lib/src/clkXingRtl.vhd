-------------------------------------------------------------------------------
--! @file clkXingRtl.vhd
--
--! @brief Clock Crossing Bus converter
--
--! @details Used to transfer a faster slave interface to a slower one.
--
-------------------------------------------------------------------------------
--
--    (c) B&R, 2013
--
--    Redistribution and use in source and binary forms, with or without
--    modification, are permitted provided that the following conditions
--    are met:
--
--    1. Redistributions of source code must retain the above copyright
--       notice, this list of conditions and the following disclaimer.
--
--    2. Redistributions in binary form must reproduce the above copyright
--       notice, this list of conditions and the following disclaimer in the
--       documentation and/or other materials provided with the distribution.
--
--    3. Neither the name of B&R nor the names of its
--       contributors may be used to endorse or promote products derived
--       from this software without prior written permission. For written
--       permission, please contact office@br-automation.com
--
--    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
--    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
--    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
--    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
--    COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
--    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
--    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
--    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
--    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
--    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
--    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
--    POSSIBILITY OF SUCH DAMAGE.
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
--! need reduce or operation
use ieee.std_logic_misc.OR_REDUCE;

library libcommon;
use libcommon.global.all;

entity clkXing is
    generic (
        gCsNum : natural := 2;
        gDataWidth : natural := 32
    );
    port (
        iArst : in std_logic;
        --fast
        iFastClk : in std_logic;
        iFastCs : in std_logic_vector(gCsNum-1 downto 0);
        iFastRNW : in std_logic;
        oFastReaddata : out std_logic_vector(gDataWidth-1 downto 0);
        oFastWrAck : out std_logic;
        oFastRdAck : out std_logic;
        --slow
        iSlowClk : in std_logic;
        oSlowCs : out std_logic_vector(gCsNum-1 downto 0);
        oSlowRNW : out std_logic;
        iSlowReaddata : in std_logic_vector(gDataWidth-1 downto 0);
        iSlowWrAck : in std_logic;
        iSlowRdAck : in std_logic
    );
end entity;

architecture rtl of clkXing is
    signal slowCs : std_logic_vector(gCsNum-1 downto 0);
    signal anyCs : std_logic;
    signal slowRnw : std_logic;
    signal wr, wr_s, wr_rising : std_logic;
    signal rd, rd_s, rd_rising : std_logic;
    signal readRegister : std_logic_vector(gDataWidth-1 downto 0);
    signal fastWrAck, fastRdAck, fastAnyAck, slowAnyAck : std_logic;
begin
    -- WELCOME TO SLOW CLOCK DOMAIN --
    genThoseCs : for i in slowCs'range generate
    begin
        theSyncCs : entity libcommon.synchronizer
            generic map (
                gStages => 2,
                gInit   => cInactivated
            )
            port map (
                iArst   => iArst,
                iClk    => iSlowClk,
                iAsync  => iFastCs(i),
                oSync   => slowCs(i)
            );
    end generate;

    anyCs <= OR_REDUCE(slowCs);

    wr_s <= anyCs and not(slowRnw) and not slowAnyAck;
    rd_s <= anyCs and slowRnw and not slowAnyAck;

    process(iArst, iSlowClk)
    begin
        if iArst = '1' then
            readRegister <= (others => '0');
            wr <= '0';
            rd <= '0';
        elsif rising_edge(iSlowClk) then
            if rd = '1' and iSlowRdAck = '1' then
                readRegister <= iSlowReaddata;
            end if;

            if iSlowWrAck = '1' then
                wr <= '0';
            elsif wr_rising = '1' then
                wr <= '1';
            end if;

            if iSlowRdAck = '1' then
                rd <= '0';
            elsif rd_rising = '1' then
                rd <= '1';
            end if;
        end if;
    end process;

    oSlowCs <= slowCs when wr = '1' or rd = '1' else (others => '0');
    oSlowRNW <= rd;

    theWriteEdge : entity libcommon.edgedetector
        port map (
            iArst       => iArst,
            iClk        => iSlowClk,
            iEnable     => cActivated,
            iData       => wr_s,
            oRising     => wr_rising,
            oFalling    => open,
            oAny        => open
        );

    theReadEdge : entity libcommon.edgedetector
        port map (
            iArst       => iArst,
            iClk        => iSlowClk,
            iEnable     => cActivated,
            iData       => rd_s,
            oRising     => rd_rising,
            oFalling    => open,
            oAny        => open
        );

    theSyncRnw : entity libcommon.synchronizer
        generic map (
            gStages => 2,
            gInit   => cInactivated
        )
        port map (
            iArst   => iArst,
            iClk    => iSlowClk,
            iAsync  => iFastRNW,
            oSync   => slowRnw
        );

    theSyncAnyAck : entity libcommon.syncTog
        generic map (
            gStages => 2,
            gInit   => cInactivated
        )
        port map (
            iSrc_rst    => iArst,
            iSrc_clk    => iFastClk,
            iSrc_data   => fastAnyAck,
            iDst_rst    => iArst,
            iDst_clk    => iSlowClk,
            oDst_data   => slowAnyAck
        );

    -- WELCOME TO FAST CLOCK DOMAIN --

    process(iArst, iFastClk)
    begin
        if iArst = '1' then
            fastAnyAck <= '0';
        elsif rising_edge(iFastClk) then
            fastAnyAck <= fastWrAck or fastRdAck;
        end if;
    end process;

    theSyncWrAck : entity libcommon.syncTog
        generic map (
            gStages => 2,
            gInit   => cInactivated
        )
        port map (
            iSrc_rst    => iArst,
            iSrc_clk    => iSlowClk,
            iSrc_data   => iSlowWrAck,
            iDst_rst    => iArst,
            iDst_clk    => iFastClk,
            oDst_data   => fastWrAck
        );

    oFastWrAck <= fastWrAck;

    theSyncRdAck : entity libcommon.syncTog
        generic map (
            gStages => 2,
            gInit   => cInactivated
        )
        port map (
            iSrc_rst    => iArst,
            iSrc_clk    => iSlowClk,
            iSrc_data   => iSlowRdAck,
            iDst_rst    => iArst,
            iDst_clk    => iFastClk,
            oDst_data   => fastRdAck
        );

    oFastRdAck <= fastRdAck;

    genThoseRdq : for i in readRegister'range generate
    begin
        theSyncRdq : entity libcommon.synchronizer
            generic map (
                gStages => 2,
                gInit   => cInactivated
            )
            port map (
                iArst   => iArst,
                iClk    => iFastClk,
                iAsync  => readRegister(i),
                oSync   => oFastReaddata(i)
            );
    end generate;
end architecture;
