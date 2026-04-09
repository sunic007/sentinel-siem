import React, { useEffect, useState, useCallback } from 'react';
import { Card, Col, Row, Statistic, Typography, Table, Tag, Button, Space } from 'antd';
import {
  AlertOutlined,
  CheckCircleOutlined,
  WarningOutlined,
  ClockCircleOutlined,
  ReloadOutlined,
} from '@ant-design/icons';
import ReactECharts from 'echarts-for-react';

const { Title } = Typography;

interface Notable {
  id: string;
  rule_id: string;
  rule_name: string;
  severity: string;
  title: string;
  description: string;
  mitre: string[];
  risk_score: number;
  status: string;
  source: string;
  host: string;
  created_at: string;
}

interface NotableStats {
  total: number;
  open: number;
  by_severity: Record<string, number>;
}

const severityColors: Record<string, string> = {
  critical: 'red',
  high: 'orange',
  medium: 'gold',
  low: 'green',
  info: 'blue',
};

const statusColors: Record<string, string> = {
  new: 'red',
  in_progress: 'orange',
  resolved: 'green',
  closed: 'default',
};

const SecurityPosture: React.FC = () => {
  const [totalEvents, setTotalEvents] = useState(0);
  const [notables, setNotables] = useState<Notable[]>([]);
  const [stats, setStats] = useState<NotableStats>({ total: 0, open: 0, by_severity: {} });
  const [resolvedToday] = useState(0);
  const [loading, setLoading] = useState(false);

  const fetchData = useCallback(async () => {
    setLoading(true);
    try {
      // Engine total events
      const healthResp = await fetch('/api/health');
      if (healthResp.ok) {
        const health = await healthResp.json();
        setTotalEvents(Number(health.engine?.total_events_ingested || 0));
      }

      // Notable stats
      const statsResp = await fetch('/api/notables/stats');
      if (statsResp.ok) {
        const s: NotableStats = await statsResp.json();
        setStats(s);
      }

      // Recent notables
      const notablesResp = await fetch('/api/notables/?limit=50');
      if (notablesResp.ok) {
        const data: Notable[] = await notablesResp.json();
        setNotables(data);
      }
    } catch (_) {
      // Engine not running — leave previous state
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    fetchData();
    // Auto-refresh every 30 seconds
    const timer = setInterval(fetchData, 30_000);
    return () => clearInterval(timer);
  }, [fetchData]);

  // Events over time — static placeholder (would need timeseries endpoint)
  const eventsOverTimeOption = {
    tooltip: { trigger: 'axis' },
    xAxis: {
      type: 'category',
      data: ['00:00', '04:00', '08:00', '12:00', '16:00', '20:00'],
      axisLabel: { color: '#888' },
    },
    yAxis: {
      type: 'value',
      axisLabel: { color: '#888' },
      splitLine: { lineStyle: { color: '#303030' } },
    },
    series: [
      {
        name: 'Events',
        type: 'line',
        smooth: true,
        data: [820, 932, 1200, 1534, 1290, 1100],
        areaStyle: { opacity: 0.15 },
        lineStyle: { color: '#1668dc' },
        itemStyle: { color: '#1668dc' },
      },
    ],
    grid: { left: 50, right: 20, top: 20, bottom: 30 },
    backgroundColor: 'transparent',
  };

  // Severity pie — from live stats
  const severityPalette: Record<string, string> = {
    critical: '#ff4d4f',
    high: '#ff7a45',
    medium: '#ffc53d',
    low: '#52c41a',
    info: '#1668dc',
  };

  const severityPieData = Object.entries(stats.by_severity).map(([name, value]) => ({
    value,
    name,
    itemStyle: { color: severityPalette[name] ?? '#888' },
  }));

  const severityOption = {
    tooltip: { trigger: 'item' },
    series: [
      {
        type: 'pie',
        radius: ['40%', '70%'],
        data: severityPieData.length > 0
          ? severityPieData
          : [{ value: 1, name: 'No data', itemStyle: { color: '#303030' } }],
        label: { color: '#ccc' },
      },
    ],
    backgroundColor: 'transparent',
  };

  const columns = [
    {
      title: 'Time',
      dataIndex: 'created_at',
      key: 'created_at',
      width: 180,
      render: (ts: string) => new Date(ts).toLocaleString(),
    },
    { title: 'Rule', dataIndex: 'title', key: 'title' },
    {
      title: 'Severity',
      dataIndex: 'severity',
      key: 'severity',
      width: 100,
      render: (s: string) => (
        <Tag color={severityColors[s]}>{s.toUpperCase()}</Tag>
      ),
    },
    {
      title: 'Status',
      dataIndex: 'status',
      key: 'status',
      width: 120,
      render: (s: string) => (
        <Tag color={statusColors[s]}>{s.replace('_', ' ').toUpperCase()}</Tag>
      ),
    },
    { title: 'Host', dataIndex: 'host', key: 'host', width: 160 },
    {
      title: 'MITRE',
      dataIndex: 'mitre',
      key: 'mitre',
      width: 200,
      render: (mitre: string[]) => (
        <Space wrap>
          {mitre.map((t) => (
            <Tag key={t} style={{ fontFamily: 'monospace' }}>{t}</Tag>
          ))}
        </Space>
      ),
    },
  ];

  return (
    <div style={{ padding: 16 }}>
      <div style={{ display: 'flex', alignItems: 'center', marginBottom: 24, gap: 12 }}>
        <Title level={3} style={{ color: '#fff', margin: 0 }}>
          Security Posture Dashboard
        </Title>
        <Button
          icon={<ReloadOutlined />}
          loading={loading}
          onClick={fetchData}
          size="small"
        >
          Refresh
        </Button>
      </div>

      <Row gutter={[16, 16]}>
        <Col span={6}>
          <Card style={{ background: '#1a1a1a', border: '1px solid #303030' }}>
            <Statistic
              title="Total Events Ingested"
              value={totalEvents}
              prefix={<ClockCircleOutlined />}
              valueStyle={{ color: '#1668dc' }}
            />
          </Card>
        </Col>
        <Col span={6}>
          <Card style={{ background: '#1a1a1a', border: '1px solid #303030' }}>
            <Statistic
              title="Notable Events"
              value={stats.total}
              prefix={<AlertOutlined />}
              valueStyle={{ color: '#ff7a45' }}
            />
          </Card>
        </Col>
        <Col span={6}>
          <Card style={{ background: '#1a1a1a', border: '1px solid #303030' }}>
            <Statistic
              title="Open Incidents"
              value={stats.open}
              prefix={<WarningOutlined />}
              valueStyle={{ color: '#ffc53d' }}
            />
          </Card>
        </Col>
        <Col span={6}>
          <Card style={{ background: '#1a1a1a', border: '1px solid #303030' }}>
            <Statistic
              title="Resolved Today"
              value={resolvedToday}
              prefix={<CheckCircleOutlined />}
              valueStyle={{ color: '#52c41a' }}
            />
          </Card>
        </Col>
      </Row>

      <Row gutter={[16, 16]} style={{ marginTop: 16 }}>
        <Col span={16}>
          <Card
            title="Events Over Time"
            style={{ background: '#1a1a1a', border: '1px solid #303030' }}
            headStyle={{ color: '#fff', borderBottom: '1px solid #303030' }}
          >
            <ReactECharts option={eventsOverTimeOption} style={{ height: 250 }} />
          </Card>
        </Col>
        <Col span={8}>
          <Card
            title="Notable Events by Severity"
            style={{ background: '#1a1a1a', border: '1px solid #303030' }}
            headStyle={{ color: '#fff', borderBottom: '1px solid #303030' }}
          >
            <ReactECharts option={severityOption} style={{ height: 250 }} />
          </Card>
        </Col>
      </Row>

      <Row style={{ marginTop: 16 }}>
        <Col span={24}>
          <Card
            title={`Recent Notable Events (${stats.total} total)`}
            style={{ background: '#1a1a1a', border: '1px solid #303030' }}
            headStyle={{ color: '#fff', borderBottom: '1px solid #303030' }}
          >
            <Table
              columns={columns}
              dataSource={notables.map((n) => ({ ...n, key: n.id }))}
              pagination={{ pageSize: 20 }}
              size="small"
              style={{ background: 'transparent' }}
              loading={loading}
              locale={{ emptyText: 'No notable events — ingest events and wait for correlation to run' }}
            />
          </Card>
        </Col>
      </Row>
    </div>
  );
};

export default SecurityPosture;
